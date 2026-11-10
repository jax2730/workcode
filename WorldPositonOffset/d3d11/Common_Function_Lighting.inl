// pbr means use roughness & metallic parameters
#define LightModel_PBR_HDR_Environment 1
#define LightModel_PBR_LDR_Environment 2

half3 DecodeEmissionR5G6B5(half2 encoded) {
  uint2 iEncoded = uint2(255 * encoded);
  uint3 iDecoded =
      uint3((iEncoded.x >> 3) & 0x1F,
            ((iEncoded.x << 3) & 0x38) + ((iEncoded.y >> 5) & 0x07),
            iEncoded.y & 0x1F);
  return half3(iDecoded) / half3(31.h, 63.h, 31.h);
}

float2 OctWrap(float2 v) {
  return (1.0 - abs(v.yx)) * (v.xy >= 0.0 ? 1.0 : -1.0);
}

float3 DecodeNormal(float3 i) {
  uint3 origin = uint3(i * 255);
  uint2 N = uint2(((origin.x & 0xff) << 4) + ((origin.y & 0xf0) >> 4),
                  ((origin.y & 0x0f) << 8) + (origin.z & 0xff));
  float2 encN = N / 4095.0; // 2^12

  encN = encN * 2.0 - 1.0;

  float3 n = float3(encN.x, encN.y, 1.0 - abs(encN.x) - abs(encN.y));
  float t = saturate(-n.z);
  n.xy += n.xy >= 0.0 ? -t : t;
  return normalize(n);
}

float3 RGBfromYUV(float3 YUV) {
  float3 colorTrans[3];
  colorTrans[0] = float3(1.000, 0.000, 1.400);
  colorTrans[1] = float3(1.000, -0.343, -0.711);
  colorTrans[2] = float3(1.000, 1.765, 0.000);

  YUV.y -= 0.5;
  YUV.z -= 0.5;
  float3 RGB;
  RGB.r = dot(colorTrans[0], YUV);
  RGB.g = dot(colorTrans[1], YUV);
  RGB.b = dot(colorTrans[2], YUV);
  return RGB;
}

struct ShadingModelInfo {
  bool emptyGBuffer;
  bool useRoleMainLight;
  bool receiveShadow;
  bool useEnvironmentMap;
  bool usePointLight;
  int lightModel; // brdf = standered pbr lighting
};

ShadingModelInfo DecodeShadingModel(float v) {
  uint code = uint(v * 255.f);
  ShadingModelInfo shadingModel;
  shadingModel.emptyGBuffer = (code == 0);
  shadingModel.useRoleMainLight = (code & 0x01) == 0x01;
  shadingModel.receiveShadow = (code & 0x02) == 0x02;
  shadingModel.useEnvironmentMap = (code & 0x04) == 0x04;
  shadingModel.usePointLight = (code & 0x08) == 0x08;
  shadingModel.lightModel = ((code >> 4) & 0xF); // higher 4 bits
  return shadingModel;
}

float2 fogExp3(in float3 cameraToWorldPos, in float2 heightScale,
               in float2 volFogHeightDensity, in float4 fogRamp) {
  float2 fogInt = float2(1.0f, 1.0f);
  static const float c_slopeThreshold = 0.01f;
  float2 top = heightScale * cameraToWorldPos.y;
  if (abs(top.x) > c_slopeThreshold) {
    fogInt.x *= (exp(top.x) - 1.0f) / top.x;
  }

  if (abs(top.y) > c_slopeThreshold) {
    fogInt.y *= (exp(top.y) - 1.0f) / top.y;
  }

  float l = length(cameraToWorldPos);
  float2 len = float2(l, l);
  float2 u = len * volFogHeightDensity;
  fogInt *= u;

  float2 fFactor = saturate(exp2(-fogInt));

  float2 vRampDen = saturate(len * fogRamp.xy - fogRamp.zw);
  vRampDen = vRampDen * (2.f - vRampDen);
  fFactor = vRampDen - fFactor * vRampDen;

  return fFactor;
}

float4 ComputeFogColor(in float3 cameraToWorldPos, in float2 heightScale,
                       in float2 volFogHeightDensity, in float4 fogRamp,
                       in float4 vFogColorNear, in float4 vFogColorFar) {
  float2 fFactor =
      fogExp3(cameraToWorldPos, heightScale, volFogHeightDensity, fogRamp);
  float2 fFactorDiff = 1.0f - fFactor;
  float4 fogColor =
      fFactor.x * fFactorDiff.y * vFogColorNear + fFactor.y * vFogColorFar;
  fogColor.a = fFactorDiff.x * fFactorDiff.y;
  return fogColor;
}
