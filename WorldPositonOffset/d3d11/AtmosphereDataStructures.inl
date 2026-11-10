#ifdef REVERSE_DEPTH
#define FAR_DEPTH_VALUE 0.f
#else
#define FAR_DEPTH_VALUE 1.f
#endif

#define PI 3.1415926535897932384626433832795f

// alignas 16
// 内存保证4个float一组，不要用float3
struct AtmosphereParameters {
  float topRadius;
  float bottomRadius;
  float minSunZenithAngleCos;
  float sunAngularRadius;

  float rayleighExp;
  float mieExp;
  float miePhaseG;
  float padding;

  float4 rayleighScattering; // xyz : color Value, w: padding
  float4 mieScattering;      // xyz : color Value, w: padding
  float4 mieExtinction;      // xyz : color Value, w: padding

  float4 groundAlbedo_multiScatteringFactor; // xyz : ground albedo
                                             // w   : multiscattering factor

  float4 ozoExtinction_width; // xyz : ozo extinction, w: ozo 0 layer width
  float4 ozoTerm;             // xy: ozo0 linear_constant term,
                              // zw: ozo1 linear_constant term
};

// alignas 16
// 内存保证4个float一组，不要用float3
struct SkyAtmosphereSetting {
  // ray marching data
  float rayMarchingSampleCountMin;
  float rayMarchingSampleCountMax;
  float rayMarchingDistanceToSampleCountMaxInv;
  float rayMarchingPadding;

  // sky view lut data
  float4 skyViewLutSizeAndInvSize;
  float skyViewSampleCountMin;
  float skyViewSampleCountMax;
  float skyViewDistanceToSampleCountMaxInv;
  float skyViewPadding;

  // camera aerial perspective volume data
  float4 aerialPerspectiveVolumeScreenSizeAndInvSize; // xy resolution and inv
  float2 aerialPerspectiveVolumeDepthResolutionAndInvResolution; // z resolution
                                                                 // and inv
  float2
      aerialPerspectiveVolumeDepthSliceLengthAndInvLength; // length in scene
                                                           // per slice and inv
  float aerialPerspectiveSampleCountPerSlice;
  float aerialPerspectivePadding0;
  float aerialPerspectivePadding1;
  float aerialPerspectivePadding2;

  // transmittance and multiple scattering
  float4 transmittanceLutSizeAndInvSize;
  float4 multiScatteredLuminanceLutSizeAndInvSize;
  float transmittanceSampleCount;   // transmittance using constant sample count
  float multiScatteringSampleCount; // multiple scattering using  constant
                                    // sample count
  float aerialPerspectiveDistanceScale;
  float padding;

  float4 skyLuminanceFactor_distanceScale;
};

struct AtmosphereViewData {
  column_major float4x4 skyViewLutReferential;
  column_major float4x4 ndcToWorldMatrix;
  column_major float4x4 cameraToWorldMatrix;
  // float4 viewMainRTRectSizeAndInvSize;
  // float4 viewMainRTRectMinAndMax;
  float4 cameraWorldPosKm_frameIdMod8;
  float4 planetCenterPosAndViewHeightKm;
  float2 cameraXYRatio;
  float2 padding;
};

struct ViewportRect {
  float4 rectMinAndMax;
  float4 rectSizeAndInvSize;
  float4 uvMinAndMax;
  float4 uvSizeAndInvSize;
};

// AtmosphereParameters GetAtmosphereParameters(float4 paramVectors[18]) {
//   AtmosphereParameters atmosphere = (AtmosphereParameters)0;
//   atmosphere.miePhaseG = paramVectors[0].w;
//   atmosphere.rayleighScattering = paramVectors[1].xyz;
//   atmosphere.muSMin = paramVectors[1].w;
//   atmosphere.mieScattering = paramVectors[2].xyz;
//   atmosphere.sunAngularRadius = paramVectors[2].w;
//   atmosphere.mieExtinction = paramVectors[3].xyz;
//   atmosphere.bottomRadius = paramVectors[3].w;
//   atmosphere.absorptionExtinction = paramVectors[4].xyz;
//   atmosphere.topRadius = paramVectors[4].w;
//   atmosphere.groundAlbedo = paramVectors[5].xyz;
//   atmosphere.multiScatteringFactor = paramVectors[5].w;

//   // rayleigh density
//   atmosphere.rayleighDensity.layers[0].width = paramVectors[6].x;
//   atmosphere.rayleighDensity.layers[0].expTerm = paramVectors[6].y;
//   atmosphere.rayleighDensity.layers[0].expScale = paramVectors[6].z;
//   atmosphere.rayleighDensity.layers[0].linearTerm = paramVectors[6].w;

//   atmosphere.rayleighDensity.layers[1].width = paramVectors[7].x;
//   atmosphere.rayleighDensity.layers[1].expTerm = paramVectors[7].y;
//   atmosphere.rayleighDensity.layers[1].expScale = paramVectors[7].z;
//   atmosphere.rayleighDensity.layers[1].linearTerm = paramVectors[7].w;

//   // mie density
//   atmosphere.mieDensity.layers[0].width = paramVectors[8].x;
//   atmosphere.mieDensity.layers[0].expTerm = paramVectors[8].y;
//   atmosphere.mieDensity.layers[0].expScale = paramVectors[8].z;
//   atmosphere.mieDensity.layers[0].linearTerm = paramVectors[8].w;

//   atmosphere.mieDensity.layers[1].width = paramVectors[9].x;
//   atmosphere.mieDensity.layers[1].expTerm = paramVectors[9].y;
//   atmosphere.mieDensity.layers[1].expScale = paramVectors[9].z;
//   atmosphere.mieDensity.layers[1].linearTerm = paramVectors[9].w;
//   return atmosphere;
// }

// SkyAtmosphereSetting GetSkyAtmosphereSetting(float4 settingVectors[10]) {
//   SkyAtmosphereSetting setting = (SkyAtmosphereSetting)0;
//   // ray marching data
//   setting.rayMarchingSampleCountMin = settingVectors[0].x;
//   setting.rayMarchingSampleCountMax = settingVectors[0].y;
//   setting.rayMarchingDistanceToSampleCountMaxInv = settingVectors[0].z;

//   // sky view lut data
//   setting.skyViewLutSizeAndInvSize = settingVectors[1];
//   setting.skyViewSampleCountMin = settingVectors[2].x;
//   setting.skyViewSampleCountMax = settingVectors[2].y;
//   setting.skyViewDistanceToSampleCountMaxInv = settingVectors[2].z;

//   // camera aerial perspective volume data
//   setting.aerialPerspectiveVolumeScreenSizeAndInvSize = settingVectors[3];
//   setting.aerialPerspectiveVolumeDepthResolutionAndInvResolution =
//       settingVectors[4].xy;
//   setting.aerialPerspectiveVolumeDepthSliceLengthAndInvLength =
//       settingVectors[4].zw;
//   setting.aerialPerspectiveSampleCountPerSlice = settingVectors[5].x;

//   // transmittance and multiple scattering
//   setting.transmittanceLutSizeAndInvSize = settingVectors[6];
//   setting.multiScatteredLuminanceLutSizeAndInvSize = settingVectors[7];
//   setting.transmittanceSampleCount = settingVectors[8].x;
//   setting.multiScatteringSampleCount = settingVectors[8].y;
//   setting.aerialPerspectiveDistanceScale = settingVectors[8].z;
//   setting.fogShowFactor = settingVectors[8].w;

//   setting.skyLuminanceFactor = settingVectors[9].xyz;
//   setting.distanceAndSizeScale = settingVectors[9].w;
//   return setting;
// }

// AtmosphereViewData GetAtmosphereViewData(float4 viewVectors[17]) {
//   AtmosphereViewData viewData = (AtmosphereViewData)0;

//   viewData.skyViewLutReferential =
//       float4x4(viewVectors[0], viewVectors[1], viewVectors[2],
//       viewVectors[3]);
//   viewData.skyViewLutReferential = transpose(viewData.skyViewLutReferential);
//   viewData.ndcToWorldMatrix =
//       float4x4(viewVectors[4], viewVectors[5], viewVectors[6],
//       viewVectors[7]);
//   viewData.ndcToWorldMatrix = transpose(viewData.ndcToWorldMatrix);
//   viewData.cameraToWorldMatrix = float4x4(viewVectors[8], viewVectors[9],
//                                           viewVectors[10], viewVectors[11]);
//   viewData.cameraToWorldMatrix = transpose(viewData.cameraToWorldMatrix);

//   viewData.viewMainRTRectSizeAndInvSize = viewVectors[12];
//   viewData.viewMainRTRectMinAndMax = viewVectors[13];
//   viewData.cameraWorldPosKm = viewVectors[14].xyz;
//   viewData.frameIdMod8 = viewVectors[14].w;
//   viewData.planetCenterPosAndViewHeightKm = viewVectors[15];
//   viewData.cameraXYRatio = viewVectors[16].xy;
//   return viewData;
// }