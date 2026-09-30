/* Typed Xbox-v5 menu metadata relocation. Layouts are the original Halo
 * headers, checked against offsetof/sizeof in halo_menu_tags.c. Unknown
 * groups, BSP payloads and GPU resource words are never scanned/rewritten. */
#include "halo_vita_cache.h"
#include "halo_vita_memory.h"
#include <stdlib.h>
#include <string.h>

#define MAX_PATCHES 65536u
#define HASH_SIZE 131072u
#define NONE 0xffffffffu
enum kind { BLOCK, DATA, REFERENCE };
enum schema { LEAF, REF, BSP, SPRITE, SEQUENCE, BITMAP, STR_ENTRY,
    STRINGS, CHAR_TABLE, FONT, INPUT, SEARCH, EVENT, CHILD, WIDGET, SCENARIO };
struct field { uint16_t offset, kind, schema; };
struct layout { uint16_t size, fields; const struct field *field; };
struct patch { uint32_t offset, before, after, pointer; };
struct vita_menu_relocation {
    unsigned char *tags;
    size_t length;
    uint32_t base, table, tag_count, count;
    struct patch *patches;
    uint32_t *hash;
    struct vita_menu_stats *stats;
    char *error;
    size_t error_size;
};
#define B(o,s) {o,BLOCK,s}
#define R(o) {o,REFERENCE,REF}
#define D(o) {o,DATA,LEAF}
static const struct field ref[] = {R(0)};
static const struct field bsp[] = {R(16)};
static const struct field seq[] = {B(52,SPRITE)};
static const struct field bitmap[] = {B(84,SEQUENCE),B(96,LEAF)};
static const struct field str_entry[] = {D(0)};
static const struct field strings[] = {B(0,STR_ENTRY)};
static const struct field char_table[] = {B(0,LEAF)};
static const struct field font[] = {B(48,CHAR_TABLE),R(60),R(76),R(92),R(108),B(124,LEAF),D(136)};
static const struct field event[] = {R(8),R(24)};
static const struct field child[] = {R(0)};
static const struct field widget[] = {R(56),B(72,INPUT),B(84,EVENT),B(96,SEARCH),
    R(236),R(252),R(340),R(356),R(420),B(724,CHILD),B(992,CHILD)};
static const struct field scenario[] = {B(48,REF),B(1444,BSP)};
#define L(sz,f) {sz,sizeof(f)/sizeof(f[0]),f}
static const struct layout layouts[] = {
    {0,0,NULL},L(16,ref),L(32,bsp),{32,0,NULL},L(64,seq),L(108,bitmap),
    L(20,str_entry),L(12,strings),L(12,char_table),L(156,font),
    {36,0,NULL},{34,0,NULL},L(72,event),L(80,child),L(1004,widget),L(1456,scenario)
};
static uint32_t read32(const unsigned char *p)
{ return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24; }
static uint16_t read16(const unsigned char *p)
{ return (uint16_t)((uint16_t)p[0] | (uint16_t)p[1]<<8); }
static void write32(unsigned char *p, uint32_t v)
{ p[0]=(unsigned char)v; p[1]=(unsigned char)(v>>8); p[2]=(unsigned char)(v>>16); p[3]=(unsigned char)(v>>24); }
static int bad(struct vita_menu_relocation *p, const char *reason, uint32_t offset)
{
    if (p->error_size) snprintf(p->error,p->error_size,"%s at tag offset0x%x",reason,offset);
    return 0;
}
static int span(struct vita_menu_relocation *p, uint32_t address, uint32_t bytes, uint32_t *offset)
{
    if (address < HALO_XBOX_TAG_BASE || address-HALO_XBOX_TAG_BASE > p->length ||
        bytes > p->length-(address-HALO_XBOX_TAG_BASE)) return 0;
    *offset=address-HALO_XBOX_TAG_BASE; return 1;
}
static int journal(struct vita_menu_relocation *p, uint32_t offset, uint32_t after, uint32_t is_pointer)
{
    uint32_t h=(offset*2654435761u)&(HASH_SIZE-1);
    while(p->hash[h]) {
        struct patch *old=&p->patches[p->hash[h]-1];
        if(old->offset==offset) return (old->after==after && old->pointer==is_pointer) || bad(p,"conflicting typed pointer/scalar",offset);
        h=(h+1)&(HASH_SIZE-1);
    }
    if(p->count==MAX_PATCHES) return bad(p,"relocation journal budget exceeded",offset);
    if(is_pointer && (offset<36 || (offset>=p->table && offset<p->table+p->tag_count*32)))
        return bad(p,"nested pointer overlaps tag directory",offset);
    p->patches[p->count]=(struct patch){offset,read32(p->tags+offset),after,is_pointer};
    p->hash[h]=++p->count; return 1;
}
static int scalar(struct vita_menu_relocation *p, uint32_t offset)
{ return journal(p,offset,read32(p->tags+offset),0); }
static int pointer(struct vita_menu_relocation *p, uint32_t slot, uint32_t bytes, uint32_t alignment, uint32_t *offset, enum kind kind)
{
    uint32_t address=read32(p->tags+slot);
    if ((address&(alignment-1)) || !span(p,address,bytes,offset))
        return bad(p,"typed pointer outside cache/misaligned",slot);
    return journal(p,slot,p->base+*offset,1+(uint32_t)kind);
}
static int reference(struct vita_menu_relocation *p, uint32_t offset)
{
    uint32_t group=read32(p->tags+offset), name=read32(p->tags+offset+4);
    uint32_t length=read32(p->tags+offset+8), index=read32(p->tags+offset+12), target;
    if(!scalar(p,offset) || !scalar(p,offset+8) || !scalar(p,offset+12)) return 0;
    if(index!=NONE) {
        uint32_t entry;
        if ((index&65535u)>=p->tag_count) return bad(p,"reference datum index out of range",offset);
        entry=p->table+(index&65535u)*32;
        if(read32(p->tags+entry+12)!=index || (group!=NONE && group!=read32(p->tags+entry) &&
            group!=read32(p->tags+entry+4) && group!=read32(p->tags+entry+8)))
            return bad(p,"reference datum/group mismatch",offset);
    }
    if(name) {
        size_t available;
        const unsigned char *end;
        if(length>255 || !pointer(p,offset+4,1,1,&target,REFERENCE)) return bad(p,"invalid reference name",offset);
        available=p->length-target;
        end=memchr(p->tags+target,0,available<256?available:256);
        /* Xbox caches retain a valid name pointer with name_length=0. */
        if(!end || (length && (size_t)(end-p->tags-target)!=length))
            return bad(p,"invalid reference name length/terminator",offset);
    } else if(length) return bad(p,"reference name absent",offset);
    ++p->stats->references; return 1;
}
static int walk(struct vita_menu_relocation *p, uint32_t offset, enum schema schema, unsigned depth);
static int block(struct vita_menu_relocation *p, uint32_t offset, enum schema child,
    uint32_t element_size, unsigned depth)
{
    uint32_t count=read32(p->tags+offset), data, i;
    if(!scalar(p,offset) || !scalar(p,offset+8)) return 0;
    if(count>32767 || read32(p->tags+offset+8)) return bad(p,"invalid block count/definition",offset);
    if(!count) return 1; /* Empty block pointers are not dereferenced by Halo. */
    if(!element_size || !pointer(p,offset+4,count*element_size,element_size==2?2:4,&data,BLOCK)) return 0;
    ++p->stats->blocks;
    if(child!=LEAF) for(i=0;i<count;++i)
        if(!walk(p,data+i*element_size,child,depth+1)) return 0;
    return 1;
}
static int walk(struct vita_menu_relocation *p, uint32_t offset, enum schema schema, unsigned depth)
{
    const struct layout *layout=&layouts[schema];
    uint32_t i, data;
    if(depth>8 || offset>p->length || layout->size>p->length-offset)
        return bad(p,"typed structure bounds/depth",offset);
    for(i=0;i<layout->fields;++i) {
        const struct field *f=&layout->field[i];
        uint32_t field=offset+f->offset;
        if(f->kind==REFERENCE) { if(!reference(p,field)) return 0; }
        else if(f->kind==DATA) {
            uint32_t size=read32(p->tags+field);
            if(!scalar(p,field) || !scalar(p,field+4) || !scalar(p,field+8) || !scalar(p,field+16)) return 0;
            if(size>p->length || read32(p->tags+field+16)) return bad(p,"invalid data size/definition",field);
            if(size && !pointer(p,field+12,size,schema==STR_ENTRY?2:1,&data,DATA)) return 0;
            if(size && schema==STR_ENTRY && (size&1)) return bad(p,"odd UTF16 string size",field);
            if(size && schema==STR_ENTRY && read16(p->tags+data+size-2))
                return bad(p,"UTF16 string is not terminated",field);
        } else {
            uint32_t stride=layouts[f->schema].size;
            if(schema==BITMAP && f->offset==96) stride=48;
            if(schema==CHAR_TABLE) stride=2;
            if(schema==FONT && f->offset==124) stride=20;
            if(!block(p,field,(enum schema)f->schema,stride,depth)) return 0;
        }
    }
    if(schema==CHAR_TABLE) {
        uint32_t count=read32(p->tags+offset);
        if(count && count!=256) return bad(p,"font lookup table is not256 entries",offset);
    }
    if(schema==FONT) {
        uint32_t tables=read32(p->tags+offset+48), chars=read32(p->tags+offset+124);
        uint32_t table_data, t;
        if(tables && !span(p,read32(p->tags+offset+52),tables*12,&table_data)) return 0;
        for(t=0;t<tables;++t) {
            uint32_t table=table_data+t*12, indices, n;
            if(!read32(p->tags+table)) continue;
            if(!span(p,read32(p->tags+table+4),512,&indices)) return 0;
            for(n=0;n<256;++n) {
                uint32_t index=read16(p->tags+indices+n*2);
                if(index!=65535u && index>=chars) return bad(p,"font character index out of bounds",indices+n*2);
            }
        }
    }
    if(schema==BITMAP && read32(p->tags+offset+96)) {
        uint32_t count=read32(p->tags+offset+96), b;
        if(!span(p,read32(p->tags+offset+100),count*48,&data)) return 0;
        for(b=0;b<count;++b) {
            const unsigned char *bitmap=p->tags+data+b*48;
            if(read32(bitmap)!=0x6269746du || !read16(bitmap+4) || !read16(bitmap+6) || !read16(bitmap+8))
                return bad(p,"bitmap signature/dimensions",data+b*48);
            ++p->stats->bitmap_count;
            if(read16(bitmap+10)==1) ++p->stats->volume_count;
        }
    }
    return 1;
}
static int menu_graph(struct vita_menu_relocation *p, uint32_t index, unsigned depth, unsigned char *state)
{
    uint32_t entry=p->table+(index&65535u)*32, root, i;
    static const uint32_t child_blocks[2]={724,992};
    if(depth>32 || state[index&65535u]==1) return bad(p,"recursive menu child cycle/depth",entry);
    if(state[index&65535u]==2) return 1;
    if(read32(p->tags+entry)!=0x44654c61u ||
        !span(p,read32(p->tags+entry+20),1004,&root)) return bad(p,"menu child is not a widget",entry);
    state[index&65535u]=1;++p->stats->menu_widgets;
    for(i=0;i<2;++i) {
        uint32_t b=root+child_blocks[i], count=read32(p->tags+b), data, n;
        if(!count) continue;
        if(!span(p,read32(p->tags+b+4),count*80,&data)) return bad(p,"menu children outside cache",b);
        for(n=0;n<count;++n) {
            uint32_t child_index=read32(p->tags+data+n*80+12);
            if(child_index!=NONE && !menu_graph(p,child_index,depth+1,state)) return 0;
        }
    }
    state[index&65535u]=2;return 1;
}
struct vita_menu_relocation *vita_cache_relocate_menu(void *tags, size_t length,
    uint32_t native_base, struct vita_menu_stats *stats, char *error, size_t error_size)
{
    struct vita_cache_info info;
    struct vita_menu_relocation *p;
    uint32_t i;
    if(error_size) error[0]=0;
    if(!native_base || (native_base&3) || length>UINT32_MAX-native_base) {
        if(error_size) snprintf(error,error_size,"invalid native cache base/alignment/overflow");
        return NULL;
    }
    if(!vita_cache_validate_index(tags,length,&info,error,error_size)) return NULL;
    memset(stats,0,sizeof(*stats)); stats->menu_index=NONE;
    p=calloc(1,sizeof(*p));
    if(!p) { if(error_size) snprintf(error,error_size,"relocation context allocation failed"); return NULL; }
    p->tags=tags;p->length=length;p->base=native_base;p->stats=stats;p->error=error;p->error_size=error_size;
    p->patches=calloc(MAX_PATCHES,sizeof(*p->patches));p->hash=calloc(HASH_SIZE,sizeof(*p->hash));
    if(!p->patches || !p->hash) { bad(p,"relocation journal allocation failed",0); goto failed; }
    p->table=read32(p->tags)-HALO_XBOX_TAG_BASE;p->tag_count=info.tag_count;
    for(i=0;i<info.tag_count;++i) {
        uint32_t entry=p->table+i*32, group=read32(p->tags+entry), root, size=1;
        enum schema schema=LEAF;
        if(group==0x44654c61u) {schema=WIDGET; ++stats->widgets;}
        else if(group==0x6269746du) {schema=BITMAP;++stats->bitmap_groups;}
        else if(group==0x666f6e74u) {schema=FONT;++stats->fonts;}
        else if(group==0x75737472u) {schema=STRINGS;++stats->string_lists;}
        else if(group==0x73636e72u) schema=SCENARIO;
        if(schema==LEAF) continue;
        size=layouts[schema].size;
        if(!span(p,read32(p->tags+entry+20),size,&root) || !walk(p,root,schema,0)) {
            if(error_size && !error[0]) bad(p,"known tag root outside cache",entry);
            goto failed;
        }
        if(schema==WIDGET) {
            uint32_t name=read32(p->tags+entry+16)-HALO_XBOX_TAG_BASE;
            if(!strcmp((const char *)p->tags+name,"ui\\shell\\main_menu\\main_menu")) stats->menu_index=read32(p->tags+entry+12);
        }
    }
    if(stats->menu_index==NONE) {bad(p,"original Main Menu widget not present",0);goto failed;}
    {
        unsigned char *state=calloc(info.tag_count,1);
        int valid;
        if(!state) {bad(p,"menu graph allocation failed",0);goto failed;}
        valid=menu_graph(p,stats->menu_index,0,state);free(state);
        if(!valid) goto failed;
    }
    for(i=0;i<p->count;++i) {
        if(p->patches[i].pointer) ++stats->pointers;
        if(p->patches[i].pointer==BLOCK+1) ++stats->block_addresses;
        if(p->patches[i].pointer==DATA+1) ++stats->data_addresses;
        if(p->patches[i].pointer==REFERENCE+1) ++stats->reference_names;
    }
    /* Commit only after every known structure/reference has been validated. */
    for(i=0;i<p->count;++i) write32(p->tags+p->patches[i].offset,p->patches[i].after);
    free(p->hash);p->hash=NULL;return p;
failed:
    free(p->hash);free(p->patches);free(p);return NULL;
}
void vita_cache_restore_menu(struct vita_menu_relocation *p)
{
    uint32_t i;
    if(!p)return;
    for(i=0;i<p->count;++i) write32(p->tags+p->patches[i].offset,p->patches[i].before);
    free(p->patches);free(p->hash);free(p);
}
