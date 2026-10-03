#!/usr/bin/env python3
"""Execute reused decoder/mixer/packet callbacks; no hardware audibility claim."""
from pathlib import Path
import subprocess
root=Path(__file__).resolve().parents[1]
s=(root/'port/linux/src/dsound_sdl.c').read_text()
body=s[s.index('#define OUTPUT_RATE'):s.index('/* ---------- output */')]
complete=s[s.index('static void packet_release('):s.index('/* ---------- stream interface */')]
interface=s[s.index('static struct sdl_stream *stream_from_interface('):s.index('static IDirectSoundStreamVtbl stream_vtable')]
start=s[s.index('static void audio_start('):s.index('#ifdef HALO_VITA\nvoid halo_vita_audio_mixer_shutdown')]
code=r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <setjmp.h>
typedef uint32_t DWORD, ULONG;
typedef int32_t LONG, HRESULT;
typedef int BOOL;
typedef DWORD *LPDWORD;
#define TRUE 1
#define FALSE 0
#define STDMETHODCALLTYPE
#define S_OK 0
#define E_INVALIDARG (-1)
#define E_OUTOFMEMORY (-2)
#define DSBVOLUME_MIN (-10000)
#define DS3DMODE_HEADRELATIVE 1
#define DS3DMODE_DISABLE 2
#define XMO_STREAMF_FIXED_SAMPLE_SIZE 1
#define XMO_STREAMF_INPUT_ASYNC 2
#define XMO_STATUSF_ACCEPT_INPUT_DATA 1
#define XMEDIAPACKET_STATUS_SUCCESS 0
#define XMEDIAPACKET_STATUS_PENDING 1
#define XMEDIAPACKET_STATUS_FLUSHED 2
typedef struct {void *lpVtbl;} IDirectSoundStream;
typedef struct {void *pvBuffer;DWORD dwMaxSize;DWORD *pdwCompletedSize,*pdwStatus;void *prtTimestamp,*pContext,*hCompletionEvent;} XMEDIAPACKET;
typedef const XMEDIAPACKET *LPCXMEDIAPACKET;
typedef struct {DWORD dwFlags,dwInputSize;} XMEDIAINFO,*LPXMEDIAINFO;
typedef void (*LPFNXMEDIAOBJECTCALLBACK)(void *,void *,DWORD);
typedef void *LPVOID;
struct fixture_event {DWORD *status,*done;DWORD bytes;unsigned wakes;};
static BOOL SetEvent(void *handle){struct fixture_event *e=handle;assert(e && *e->status==XMEDIAPACKET_STATUS_SUCCESS && *e->done==e->bytes);e->wakes++;return TRUE;}
static int fail_allocation;
static void *fixture_malloc(size_t n){return fail_allocation?NULL:malloc(n);}
#define malloc fixture_malloc
static void platform_log(const char *f,...);
static jmp_buf failed;
static _Noreturn void vita_fatal(const char *message){(void)message;longjmp(failed,1);}
'''+body+complete+interface+r'''
/* Native audio-start behavior under a modeled SDL device owner. */
typedef int SDL_AudioStream;
typedef struct {int format,channels,freq;} SDL_AudioSpec;
#define SDL_AUDIO_F32 1
#define SDL_HINT_AUDIO_DEVICE_SAMPLE_FRAMES "frames"
#define SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK 0
static BOOL audio_started;
static SDL_AudioStream *audio_stream;
static int initialized,open_calls,resume_calls,device_failure,callback_calls;
static int config_boolean(const char *name){assert(!strcmp(name,"audio.enabled"));return 1;}
static double config_real(const char *name){assert(!strcmp(name,"audio.volume"));return 1;}
static int platform_sdl_initialize(void){initialized++;return device_failure!=1;}
static void audio_callback(void *u,SDL_AudioStream *s,int a,int t){(void)u;(void)s;(void)a;(void)t;}
static void SDL_SetHint(const char *h,const char *v){assert(!strcmp(h,"frames") && !strcmp(v,"512"));}
static SDL_AudioStream *SDL_OpenAudioDeviceStream(int d,const SDL_AudioSpec *s,void (*cb)(void *,SDL_AudioStream *,int,int),void *u){
assert(d==0 && s->freq==48000 && s->channels==2 && s->format==SDL_AUDIO_F32 && cb==audio_callback && !u);
open_calls++;return device_failure==2?NULL:(SDL_AudioStream *)&open_calls;}
static int SDL_ResumeAudioStreamDevice(SDL_AudioStream *s){assert(s);resume_calls++;return device_failure!=3;}
static const char *SDL_GetError(void){return "injected device failure";}
static const char *SDL_GetCurrentAudioDriver(void){return "mock-vita";}
static void vita_log(const char *f,...){(void)f;}
static void platform_log(const char *f,...){(void)f;}
'''+start+r'''
static pthread_t consumer_thread;
static void finished(void *context,void *packet,DWORD status){
assert(context==(void *)1 && packet==(void *)2 && status==XMEDIAPACKET_STATUS_SUCCESS);
assert(pthread_equal(pthread_self(),consumer_thread));callback_calls++;
/* callback runs outside mixer_lock, as original cache/channel code requires */
assert(!pthread_mutex_trylock(&mixer_lock));pthread_mutex_unlock(&mixer_lock);}
static void *render_audio_thread(void *arg){mix(arg,480);return NULL;}
int main(void){
assert(gain_from_millibels(0)==1.f && gain_from_millibels(-10000)==0.f);
unsigned long frames;unsigned char block[72]={0};short *pcm=decode_adpcm(block,36,1,&frames);
assert(frames==64);for(unsigned i=0;i<frames;i++)assert(!pcm[i]);free(pcm);
block[0]=0xe8;block[1]=3;block[4]=0x18;block[5]=0xfc;
pcm=decode_adpcm(block,72,2,&frames);assert(frames==64);
for(unsigned i=0;i<frames;i++){assert(pcm[2*i]==1000);assert(pcm[2*i+1]==-1000);}free(pcm);
short source[128];for(unsigned i=0;i<64;i++){source[2*i]=8192;source[2*i+1]=-8192;}
struct sdl_stream voice={0};streams=&voice;voice.reference_count=2;
voice.channels=2;voice.frequency=voice.sample_rate=44100;voice.volume=voice.mix_left=voice.mix_right=1;
voice.callback=finished;voice.context=(void *)1;
DWORD status=99,done=99;
struct fixture_event event={&status,&done,sizeof(source),0};
XMEDIAPACKET packet={source,sizeof(source),&done,&status,NULL,(void *)2,&event};
assert(stream_process(&voice.object,&packet,NULL)==S_OK && voice.packet_count==1 && !done && status==XMEDIAPACKET_STATUS_PENDING);
assert(voice.packets[0].samples!=source && !memcmp(source,voice.packets[0].samples,sizeof(source)));
/* Reuse caller/cache bytes immediately: decoded packet owns an independent copy. */
memset(source,0,sizeof(source));float output[960];pthread_t audio;
assert(!pthread_create(&audio,NULL,render_audio_thread,output));assert(!pthread_join(audio,NULL));
assert(!callback_calls && status==XMEDIAPACKET_STATUS_PENDING);
assert(output[2*16]>0.1f && output[2*16+1]<-0.1f);
consumer_thread=pthread_self();streams_complete_finished();
assert(callback_calls==1 && !voice.packet_count && done==sizeof(source) && status==XMEDIAPACKET_STATUS_SUCCESS);
assert(!event.wakes); /* Original callback takes precedence over event. */
voice.callback=NULL;
assert(stream_process(&voice.object,&packet,NULL)==S_OK);
mix(output,480);assert(!event.wakes);streams_complete_finished();
assert(event.wakes==1 && !voice.packet_count && done==sizeof(source) && status==XMEDIAPACKET_STATUS_SUCCESS);
packet.hCompletionEvent=NULL;packet.pdwStatus=NULL;packet.pdwCompletedSize=NULL;
for(unsigned i=0;i<MAXIMUM_STREAM_PACKETS;i++)assert(stream_process(&voice.object,&packet,NULL)==S_OK);
assert(stream_process(&voice.object,&packet,NULL)==E_OUTOFMEMORY);
assert(stream_get_status(&voice.object,&status)==S_OK && !status);
assert(stream_flush(&voice.object)==S_OK && !voice.packet_count);
assert(stream_add_reference(&voice.object)==3 && stream_release(&voice.object)==2);
XMEDIAINFO info;assert(stream_get_info(&voice.object,&info)==S_OK && info.dwInputSize==4);
assert(stream_discontinuity(&voice.object)==S_OK && stream_process(&voice.object,NULL,NULL)==E_INVALIDARG);
fail_allocation=1;assert(stream_process(&voice.object,&packet,NULL)==E_OUTOFMEMORY && !voice.packet_count);fail_allocation=0;
/* Successful startup is audible-device-only; each device failure stops explicitly. */
audio_start();audio_start();assert(initialized==1 && open_calls==1 && resume_calls==1);
for(int i=1;i<=3;i++){audio_started=FALSE;device_failure=i;if(!setjmp(failed)){audio_start();assert(!"silent device accepted");}}
puts("PASS actual original SDL mixer: mono/stereo Xbox ADPCM, PCM stereo/resample, independent cache packet copy, nonzero output, callback/event precedence and completion on consumer,64-packet bound/flush,device failure");}
'''
out=root/'build/vita/tests/audio-mixer';out.mkdir(parents=True,exist_ok=True)
p=out/'mixer.c';p.write_text(code);exe=out/'mixer'
subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-DHALO_VITA',str(p),'-pthread','-lm','-o',str(exe)],check=True)
subprocess.run([str(exe)],check=True)
