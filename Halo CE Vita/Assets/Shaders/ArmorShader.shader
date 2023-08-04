Shader "Halo/ArmorShader" {
    Properties{
        _ColorArmadura("Color Armadura", Color) = (1,1,1,1)
        _MainTex("Albedo (RGB)", 2D) = "white" {}
        _Multipurpose("Multipurpose (RGB)", 2D) = "white" {}
        _Cubemap("Cubemap", CUBE) = "" {}
        _CubemapPower("Cubemap Power", Range(0,1)) = 0.5
        _VisorTexture("Visor Texture", 2D) = "white" {}
        _CubemapVisor("Cubemap Visor", CUBE) = "" {}
        _ShieldColor("Shield Color", Color) = (1,1,1,1)
        _ShieldActive("Shield Active", Range(0,1)) = 0.5
        _ShieldTex("Shield Texture", 2D) = "white" {}
        _ShieldEdge("Shield Edge", Range(0,1)) = 0.5
        _ShieldTransparency("Shield Transparency", Range(0,2)) = 0.5
        _ShieldMovement("Shield Movement", Range(0,0.1)) = 0.05
    }
        SubShader{
            Tags { "RenderType" = "Opaque" }
            LOD 100

            Pass {
                CGPROGRAM
                // Upgrade NOTE: excluded shader from DX11; has structs without semantics (struct v2f members color)

                                #pragma vertex vert
                                #pragma fragment frag

                                #include "UnityCG.cginc"

                                sampler2D _MainTex;
                                sampler2D _Multipurpose;
                                samplerCUBE _Cubemap;
                                float4 _ColorArmadura;
                                float _CubemapPower;
                                sampler2D _VisorTexture;
                                samplerCUBE _CubemapVisor;
                                float4 _ShieldColor;
                                float _ShieldActive;
                                sampler2D _ShieldTex;
                                float _ShieldEdge;
                                float _ShieldTransparency;
                                float _ShieldMovement;
                            struct appdata {
                                    float4 vertex : POSITION;
                                    float2 uv : TEXCOORD0;
                                    float3 normal : NORMAL;
                            };
                            struct v2f {
                                float4 vertex : POSITION;
                                float2 uv : TEXCOORD0;
                                float3 worldRefl : TEXCOORD1;
                                float3 worldNormal : TEXCOORD2;
                                float3 worldPosition: TEXCOORD3;
                            };

                                v2f vert(appdata v) {
                                    v2f o;
                                    o.vertex = UnityObjectToClipPos(v.vertex);
                                    o.uv = v.uv;
                                    o.worldNormal = normalize(mul(v.normal, (float3x3)unity_ObjectToWorld));
                                    float3 worldView = normalize(_WorldSpaceCameraPos - mul(unity_ObjectToWorld, v.vertex).xyz);
                                    o.worldRefl = reflect(-worldView, o.worldNormal);
                                    //////This normalize function in your fragment shader was breaking it
                                    //////I think because of the _WorldSpaceCameraPos access,
                                    //////Vita has no POSITION semantic to access this in the fragmant shader 
                                    float3 worldPos = normalize(_WorldSpaceCameraPos - v.vertex.xyz);
                                    o.worldPosition = worldPos;
                                    return o;
                                }

                                fixed4 frag(v2f i) : SV_Target
                                {
                                    fixed4 col = tex2D(_MainTex, i.uv);
                                    fixed4 multipurpose = tex2D(_Multipurpose, i.uv);
                                    fixed4 visor = tex2D(_VisorTexture, i.uv);
                                    col.rgb = lerp(col.rgb, _ColorArmadura.rgb * multipurpose.r, multipurpose.b);
                                    float3 viewDir = reflect(i.worldNormal, i.worldRefl);
                                    float opacity = 1 - visor.a;
                                    col.rgb += texCUBE(_Cubemap, i.worldRefl).rgb * multipurpose.r * _CubemapPower * opacity;
                                    col.rgb += texCUBE(_CubemapVisor, viewDir).rgb * visor.a * multipurpose.r;

                                    //the commented out part below is what is causing your shader to break, it should compile fine now
                                    float fresnel = pow(1 - max(dot(i.worldNormal, i.worldPosition), 0.0f), 5.0f);
                                    fresnel = lerp(1, fresnel, _ShieldEdge);
                                    fixed4 shield = tex2D(_ShieldTex, i.uv + float2(_Time.y * _ShieldMovement, _Time.y * _ShieldMovement));
                                    shield.rgb *= _ShieldColor.rgb;
                                    col.rgb = lerp(col.rgb, shield.rgb, shield.a * _ShieldTransparency * fresnel * _ShieldActive);
                                    return col;
                                }

                                ENDCG
                            }
        }
}