Texture2D gSceneTexture : register(t0);
SamplerState gssWrap : register(s0);
SamplerState gssClamp : register(s1);

cbuffer cbPostProcess : register(b1)
{
    float gHitEffectAmount; // 피격 효과 강도 (0.0 ~ 1.0)
    float gSpeedAmount;     // 속도 효과 강도 (0.0 ~ 1.0)
    float gTime;
};

struct VS_OUTPUT
{
    float4 PosH : SV_POSITION;
    float2 TexC : TEXCOORD0;
};

VS_OUTPUT VSPostProcess(float3 pos : POSITION, float2 uv : TEXCOORD)
{
    VS_OUTPUT output;
    output.PosH = float4(pos, 1.0f);
    output.TexC = uv;
    return output;
}

float random(float2 uv, float time)
{
    return frac(sin(dot(uv, float2(12.9898, 78.233)) + time) * 43758.5453);
}

float4 PSPostProcess(VS_OUTPUT input) : SV_TARGET
{
// 속도 효과
    float4 finalColor = float4(0, 0, 0, 0);    
    // 화면 중앙에서 현재 픽셀 방향으로의 벡터
    float2 dir = input.TexC - 0.5f;
    
    // 블러 강도
    float strengthMultiplier = 0.04f; 
    
    float blurStrength = gSpeedAmount * length(dir) * strengthMultiplier;

    // 여러 번 샘플링하여 색상을 섞어 블러 효과 적용
    [unroll]
    for (int i = 0; i < 8; i++)
    {
        // 중앙에서 바깥쪽으로 조금씩 이동하며 샘플링
        float2 offset = dir * (i / 7.0f) * blurStrength;
        finalColor += gSceneTexture.Sample(gssClamp, input.TexC + offset);
    }
    finalColor /= 8.0f; // 샘플링한 색상들의 평균
    
// 피격 효과
    //  원본 씬 색상
    float4 sceneColor = gSceneTexture.Sample(gssWrap, input.TexC);
    // 화면 중앙으로부터 거리 계산
    float distFromCenter = length(input.TexC - 0.5f);    
    // 중앙은 0, 모서리는 1
    float vignette = saturate(distFromCenter * 2.0f);    
    // 피격 효과 색상
    float3 hitColor = float3(1.0f, 0.0f, 0.0f);
    // 원본 색상과 피격 색상 섞음
    float blendAmount = vignette * gHitEffectAmount;
    
    finalColor.rgb = lerp(finalColor.rgb, hitColor, blendAmount);

 // 필름 그레인 효과(미세 노이즈)   
    float noise = (random(input.TexC, gTime) - 0.5f) * 0.1f; // 0.15는 노이즈 강도

    // 최종 색상에 노이즈를 더합니다.
    finalColor.rgb += noise;
    
    return finalColor;
}