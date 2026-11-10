// #define TRANSMITTANCE_TEXTURE_WIDTH 256
// #define TRANSMITTANCE_TEXTURE_HEIGHT 64

// #define SCATTERING_TEXTURE_R_SIZE 32
// #define SCATTERING_TEXTURE_MU_SIZE 128
// #define SCATTERING_TEXTURE_MU_S_SIZE 32
// #define SCATTERING_TEXTURE_NU_SIZE 8

// #define IRRADIANCE_TEXTURE_WIDTH 64
// #define IRRADIANCE_TEXTURE_HEIGHT 16

// #define COMPUTE_GROUP_SIZE_X 16
// #define COMPUTE_GROUP_SIZE_Y 16
// #define COMPUTE_GROUP_SIZE_Z 4

#define IRRADIANCE_TEXTURE_SIZE int2(IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT)

// #include <LineCommittedInclude.h>
/* #include <BlockCommittedInclude.h>
this is a commit block!
this is a commit block!
this is a commit block!
this is a commit block!
*/

struct DensityProfileLayer {
    float width;
    float exp_term;
    float exp_scale;
    float linear_term;
    float constant_term;
};

struct DensityProfile {
    DensityProfileLayer layers[2];     
};

struct AtmosphereParameters {
    float3 solar_irradiance;
    float mie_phase_function_g;
    float3 rayleigh_scattering;
    float mu_s_min;
    float3 mie_scattering;
    float sun_angular_radius;
    float3 mie_extinction;
    float bottom_radius;
    float3 absorption_extinction;
    float top_radius;
    float3 ground_albedo;
    DensityProfile rayleigh_density;
    DensityProfile mie_density;
    DensityProfile absorption_density;
};

AtmosphereParameters initAtmosphereWithBoundary(
    float top_radius,
    float bottom_radius)
{
    DensityProfileLayer defaultLayer;
    defaultLayer.width = 0.f;       
    defaultLayer.exp_term = 0.f;
    defaultLayer.exp_scale = 0.f;    
    defaultLayer.linear_term = 0.f;
    defaultLayer.constant_term = 0.f;

    DensityProfile default_profile;
    default_profile.layers[0] = defaultLayer;
    default_profile.layers[1] = defaultLayer;

    AtmosphereParameters atmosphere;
    atmosphere.top_radius = top_radius;
    atmosphere.bottom_radius = bottom_radius;
    atmosphere.rayleigh_scattering = 0.f;
    atmosphere.mie_scattering = 0.f;
    atmosphere.mie_extinction = 0.f;
    atmosphere.absorption_extinction = 0.f;
    atmosphere.rayleigh_density = default_profile;
    atmosphere.mie_density = default_profile;
    atmosphere.absorption_density = default_profile;
    atmosphere.sun_angular_radius = 0.f;
    atmosphere.mu_s_min = 0.f;
    atmosphere.ground_albedo = 0.f;

    return atmosphere;
}

AtmosphereParameters initExponentialHeightFog(
    float top_radius, 
    float bottom_radius, 
    float exp_scale,
    float exp_term)
{
    DensityProfileLayer defaultLayer;
    defaultLayer.width = 0.f;       
    defaultLayer.exp_term = 0.f;
    defaultLayer.exp_scale = 0.f;    
    defaultLayer.linear_term = 0.f;
    defaultLayer.constant_term = 0.f;

    DensityProfileLayer fogLayer;
    fogLayer.width = 0.f;
    fogLayer.exp_term = exp_term;
    fogLayer.exp_scale = exp_scale;
    fogLayer.linear_term = 0.f;
    fogLayer.constant_term = 0.f;

    DensityProfile fogProfile;
    fogProfile.layers[0] = fogLayer;
    fogProfile.layers[1] = fogLayer;

    DensityProfile default_profile;
    default_profile.layers[0] = defaultLayer;
    default_profile.layers[1] = defaultLayer;

    AtmosphereParameters atmosphere;
    atmosphere.top_radius = top_radius;
    atmosphere.bottom_radius = bottom_radius;
    atmosphere.rayleigh_scattering = 1.f;    // use rayleigh 
    atmosphere.mie_scattering = 0.f;
    atmosphere.mie_extinction = 0.f;
    atmosphere.absorption_extinction = 0.f;
    atmosphere.rayleigh_density = fogProfile;
    atmosphere.mie_density = default_profile;
    atmosphere.absorption_density = default_profile;
    atmosphere.sun_angular_radius = 0.f;
    atmosphere.mu_s_min = 0.f;
    atmosphere.ground_albedo = 0.f;

    return atmosphere;
}

float ComputeDitherRange(in float mu)
{
    float offsetStrength = smoothstep(-0.1f, 0.1f, mu);
    return 4.f * offsetStrength;
}

float2 DitherUVTransmittanceTexture(
    in float ditherRange,
    in float2 uv, 
    in float2 screenPosition)
{
    float2 pixelSize = 1.f / float2(TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_HEIGHT);
    float2 offset = float2(ditherRange, 0.f) * pixelSize;
    const float4 D2 = 0.25f * float4(3.f, 1.f, 0.f, 2.f);
    const float4 target = float4(1.f, 2.f, 3.f, 4.f);
    float2 ij = floor(fmod(screenPosition, float2(2.f, 2.f)));

    float idx = ij.x + 2.f * ij.y;
    float4 m = step(abs(idx - target), 0.5f) * D2;
    float d = m.x + m.y + m.z + m.w;
    uv = clamp(0.f, 1.f, uv + d * offset);
    return uv;
}

float DistanceToTopAtmosphereBoundary(in AtmosphereParameters atmosphere,
    float r, float mu) {
  float discriminant = r * r * (mu * mu - 1.0) +
      atmosphere.top_radius * atmosphere.top_radius;
  return max(0, -r * mu + sqrt(max(discriminant, 0.0)));
}

float DistanceToBottomAtmosphereBoundary(in AtmosphereParameters atmosphere,
    float r, float mu) {
  float discriminant = r * r * (mu * mu - 1.0) +
      atmosphere.bottom_radius * atmosphere.bottom_radius;
  return max(0, -r * mu - sqrt(max(discriminant, 0.0)));
}

bool RayIntersectsGround(in AtmosphereParameters atmosphere,
    float r, float mu) {
  return mu < 0.0 && r * r * (mu * mu - 1.0) +
      atmosphere.bottom_radius * atmosphere.bottom_radius >= 0.0f;
}

float GetLayerDensity(in DensityProfileLayer layer, float altitude) {
  float density = layer.exp_term * exp(layer.exp_scale * altitude) +
      layer.linear_term * altitude + layer.constant_term;
  return clamp(density, 0.0f, 1.0f);
}

float GetProfileDensity(in DensityProfile profile, float altitude) {
  return altitude < profile.layers[0].width ?
      GetLayerDensity(profile.layers[0], altitude) :
      GetLayerDensity(profile.layers[1], altitude);
}

float ComputeOpticalLengthToTopAtmosphereBoundary(
    in AtmosphereParameters atmosphere, in DensityProfile profile,
    float r, float mu) 
{
    // Number of intervals for the numerical integration.
    const int SAMPLE_COUNT = 500;
    // The integration step, i.e. the length of each integration interval.
    float dx =
        DistanceToTopAtmosphereBoundary(atmosphere, r, mu) / float(SAMPLE_COUNT);
    // Integration loop.
    float result = 0.0;
    for (int i = 0; i <= SAMPLE_COUNT; ++i) 
    {
        float d_i = float(i) * dx;
        // Distance between the current sample point and the planet center.
        float r_i = sqrt(d_i * d_i + 2.0 * r * mu * d_i + r * r);
        // Number density at the current sample point (divided by the number density
        // at the bottom of the atmosphere, yielding a dimensionless number).
        float y_i = GetProfileDensity(profile, r_i - atmosphere.bottom_radius);
        // Sample weight (from the trapezoidal rule).
        float weight_i = i == 0 || i == SAMPLE_COUNT ? 0.5 : 1.0;
        result += y_i * weight_i * dx;
    }
    return result;
}

float3 ComputeTransmittanceToTopAtmosphereBoundary(
    in AtmosphereParameters atmosphere, float r, float mu) 
{
    return exp(-(
        atmosphere.rayleigh_scattering *
            ComputeOpticalLengthToTopAtmosphereBoundary(
                atmosphere, atmosphere.rayleigh_density, r, mu) +
        atmosphere.mie_extinction *
            ComputeOpticalLengthToTopAtmosphereBoundary(
                atmosphere, atmosphere.mie_density, r, mu) +
        atmosphere.absorption_extinction *
            ComputeOpticalLengthToTopAtmosphereBoundary(
                atmosphere, atmosphere.absorption_density, r, mu)));
}

float2 GetTextureCoordFromUnitRange(float2 x, int2 texture_size) {
  return 0.5 / float2(texture_size) + x * (1.0 - 1.0 / float2(texture_size));
}

float2 GetUnitRangeFromTextureCoord(float2 u, int2 texture_size) {
  return (u - 0.5 / float2(texture_size)) / (1.0 - 1.0 / float2(texture_size));
}

float GetTextureCoordFromUnitRange(float x, int texture_size) {
  return 0.5 / float(texture_size) + x * (1.0 - 1.0 / float(texture_size));
}

float GetUnitRangeFromTextureCoord(float u, int texture_size) {
  return (u - 0.5 / float(texture_size)) / (1.0 - 1.0 / float(texture_size));
}

float2 GetTransmittanceTextureUvFromRMu(in AtmosphereParameters atmosphere,
    float r, float mu) 
{
    // Distance to top atmosphere boundary for a horizontal ray at ground level.
    float H = sqrt(atmosphere.top_radius * atmosphere.top_radius -
        atmosphere.bottom_radius * atmosphere.bottom_radius);
    // Distance to the horizon.
    float rho =
        sqrt(max(r * r - atmosphere.bottom_radius * atmosphere.bottom_radius, 0));
    // Distance to the top atmosphere boundary for the ray (r,mu), and its minimum
    // and maximum values over all mu - obtained for (r,1) and (r,mu_horizon).
    float d = DistanceToTopAtmosphereBoundary(atmosphere, r, mu);
    float d_min = atmosphere.top_radius - r;
    float d_max = rho + H;
    float d_mu = (d - d_min) / (d_max - d_min);
    float d_x = rho / H;
    return GetTextureCoordFromUnitRange(float2(d_mu, d_x), int2(TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_HEIGHT));
}

void GetRMuFromTransmittanceTextureUv(in AtmosphereParameters atmosphere,
    in float2 uv, out float r, out float mu) 
{
    float2 x = GetUnitRangeFromTextureCoord(uv, int2(TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_HEIGHT));
    // Distance to top atmosphere boundary for a horizontal ray at ground level.
    float H = sqrt(atmosphere.top_radius * atmosphere.top_radius -
        atmosphere.bottom_radius * atmosphere.bottom_radius);
    // Distance to the horizon, from which we can compute r:
    float rho = H * x.y;
    r = sqrt(rho * rho + atmosphere.bottom_radius * atmosphere.bottom_radius);
    // Distance to the top atmosphere boundary for the ray (r,mu), and its minimum
    // and maximum values over all mu - obtained for (r,1) and (r,mu_horizon) -
    // from which we can recover mu:
    float d_min = atmosphere.top_radius - r;
    float d_max = rho + H;
    float d = d_min + x.x * (d_max - d_min);
    mu = d == 0.0 ? 1.0f : (H * H - rho * rho - d * d) / (2.0f * r * d);
    mu = clamp(mu, -1.0f, 1.0f);
}

float3 ComputeTransmittanceToTopAtmosphereBoundaryTexture(
    in AtmosphereParameters atmosphere, in float2 screen_pos) 
{
    const float2 TRANSMITTANCE_TEXTURE_SIZE =
        float2(TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_HEIGHT);
    float r;
    float mu;
    GetRMuFromTransmittanceTextureUv(
        atmosphere, screen_pos / TRANSMITTANCE_TEXTURE_SIZE, r, mu);
    return ComputeTransmittanceToTopAtmosphereBoundary(atmosphere, r, mu);
}

float3 GetTransmittanceToTopAtmosphereBoundaryWithDithering(
    in AtmosphereParameters atmosphere,
    in Texture2D transmittance_texture,
    in SamplerState transmittance_sampler,
    float2 screenPosition,
    float r, float mu) 
{
    float ditherRange = ComputeDitherRange(mu);
    r = clamp(r, atmosphere.bottom_radius + 1e-4f, atmosphere.top_radius - 1e-4f);
    mu = clamp(mu, -1.f, 1.f);
    float2 uv = GetTransmittanceTextureUvFromRMu(atmosphere, r, mu);
    uv = DitherUVTransmittanceTexture(ditherRange, uv, screenPosition);
    return transmittance_texture.SampleLevel(transmittance_sampler, uv, 0).xyz;
}

float3 GetTransmittanceToTopAtmosphereBoundary(
    in AtmosphereParameters atmosphere,
    in Texture2D transmittance_texture,
    in SamplerState transmittance_sampler,
    float r, float mu) 
{
    float2 uv = GetTransmittanceTextureUvFromRMu(atmosphere, r, mu);
    return transmittance_texture.SampleLevel(transmittance_sampler, uv, 0).xyz;
}

float3 GetTransmittance(
    in AtmosphereParameters atmosphere,
    in Texture2D transmittance_texture,
    in SamplerState transmittance_sampler,
    float r, float mu, float d, bool ray_r_mu_intersects_ground) 
{
    float r_d = clamp(sqrt(d * d + 2.0 * r * mu * d + r * r), atmosphere.bottom_radius, atmosphere.top_radius);
    float mu_d = clamp((r * mu + d) / r_d, -1.0, 1.0);

    if (ray_r_mu_intersects_ground) 
    {
        return min(
            GetTransmittanceToTopAtmosphereBoundary(
                atmosphere, transmittance_texture, transmittance_sampler, r_d, -mu_d) /
            GetTransmittanceToTopAtmosphereBoundary(
                atmosphere, transmittance_texture, transmittance_sampler, r, -mu),
            1.0);
    }
    else 
    {
        return min(
            GetTransmittanceToTopAtmosphereBoundary(
                atmosphere, transmittance_texture, transmittance_sampler, r, mu) /
            GetTransmittanceToTopAtmosphereBoundary(
                atmosphere, transmittance_texture, transmittance_sampler, r_d, mu_d),
            1.0);
    }
}

float3 GetTransmittanceToSun(
    in AtmosphereParameters atmosphere,
    in Texture2D transmittance_texture,
    in SamplerState transmittance_sampler,
    float r, float mu_s) {
  float sin_theta_h = atmosphere.bottom_radius / r;
  float cos_theta_h = -sqrt(max(1.0 - sin_theta_h * sin_theta_h, 0.0));
  return GetTransmittanceToTopAtmosphereBoundary(
        atmosphere, transmittance_texture, transmittance_sampler, r, mu_s) *
        smoothstep(-sin_theta_h * atmosphere.sun_angular_radius,
                 sin_theta_h * atmosphere.sun_angular_radius,
                 mu_s - cos_theta_h);
}

void ComputeSingleScatteringIntegrand(
    in AtmosphereParameters atmosphere,
    in Texture2D transmittance_texture,
    in SamplerState transmittance_sampler,
    float r, float mu, float mu_s, float nu, float d,
    bool ray_r_mu_intersects_ground,
    out float3 rayleigh, out float3 mie) 
{
    float r_d = clamp(sqrt(d * d + 2.0 * r * mu * d + r * r), atmosphere.bottom_radius, atmosphere.top_radius);
    float mu_s_d = clamp((r * mu_s + d * nu) / r_d, -1.0, 1.0);
    float3 transmittance =
        GetTransmittance(
            atmosphere, transmittance_texture, transmittance_sampler, r, mu, d,
            ray_r_mu_intersects_ground) *
        GetTransmittanceToSun(
            atmosphere, transmittance_texture, transmittance_sampler, r_d, mu_s_d);

    // f(d) = T(sun, q) * T(q, p)
    // rayleigh = f(rd) * delta_rd
    // mie = f(md) * delta_md
    rayleigh = transmittance * GetProfileDensity(
        atmosphere.rayleigh_density, r_d - atmosphere.bottom_radius);
    mie = transmittance * GetProfileDensity(
        atmosphere.mie_density, r_d - atmosphere.bottom_radius);
}

float DistanceToNearestAtmosphereBoundary(in AtmosphereParameters atmosphere,
    float r, float mu, bool ray_r_mu_intersects_ground) 
{
    if (ray_r_mu_intersects_ground) 
    {
        return DistanceToBottomAtmosphereBoundary(atmosphere, r, mu);
    } 
    else 
    {
        return DistanceToTopAtmosphereBoundary(atmosphere, r, mu);
    }
}

void ComputeSingleScattering(
    in AtmosphereParameters atmosphere,
    in Texture2D transmittance_texture,
    in SamplerState transmittance_sampler,
    float r, float mu, float mu_s, float nu,
    bool ray_r_mu_intersects_ground,
    out float3 rayleigh, out float3 mie) 
{
    // Number of intervals for the numerical integration.
    const int SAMPLE_COUNT = 50;
    // The integration step, i.e. the length of each integration interval.
    float dx =
        DistanceToNearestAtmosphereBoundary(atmosphere, r, mu,
            ray_r_mu_intersects_ground) / float(SAMPLE_COUNT);
    // Integration loop.
    float3 rayleigh_sum = 0.0f;
    float3 mie_sum = 0.0f;
    for (int i = 0; i <= SAMPLE_COUNT; ++i) 
    {
        float d_i = i * dx;
        // The Rayleigh and Mie single scattering at the current sample point.
        float3 rayleigh_i;
        float3 mie_i;
        ComputeSingleScatteringIntegrand(atmosphere, transmittance_texture, transmittance_sampler,
            r, mu, mu_s, nu, d_i, ray_r_mu_intersects_ground, rayleigh_i, mie_i);
        // Sample weight (from the trapezoidal rule).
        float weight_i = (i == 0 || i == SAMPLE_COUNT) ? 0.5 : 1.0;
        rayleigh_sum += rayleigh_i * weight_i;
        mie_sum += mie_i * weight_i;
    }
    rayleigh = rayleigh_sum * dx * /*atmosphere.solar_irradiance **/
        atmosphere.rayleigh_scattering;
    mie = mie_sum * dx * /*atmosphere.solar_irradiance **/ atmosphere.mie_scattering;
}

float RayleighPhaseFunction(float nu) {
    // k = 3 / (16 * PI);
    const float k = 0.05968310365946075091333141126469f;
    return k * (1.0 + nu * nu);
}

float MiePhaseFunction(float g, float nu) {
    // c = 3 / (8 * PI);
    const float c = 0.11936620731892150182666282252938;
    float k = c * (1.0 - g * g) / (2.0 + g * g);
    return k * (1.0 + nu * nu) / pow(1.0 + g * g - 2.0 * g * nu, 1.5);
}

float4 GetScatteringTextureUvwzFromRMuMuSNu(in AtmosphereParameters atmosphere,
    float r, float mu, float mu_s, float nu,
    bool ray_r_mu_intersects_ground) 
{
    // Distance to top atmosphere boundary for a horizontal ray at ground level.
    float H = sqrt(atmosphere.top_radius * atmosphere.top_radius -
        atmosphere.bottom_radius * atmosphere.bottom_radius);
    // Distance to the horizon.
    float rho = sqrt(max(0, r * r - atmosphere.bottom_radius * atmosphere.bottom_radius));
    float u_r = GetTextureCoordFromUnitRange(rho / H, SCATTERING_TEXTURE_R_SIZE);

    // Discriminant of the quadratic equation for the intersections of the ray
    // (r,mu) with the ground (see RayIntersectsGround).
    float r_mu = r * mu;
    float discriminant =
        r_mu * r_mu - r * r + atmosphere.bottom_radius * atmosphere.bottom_radius;
    float u_mu;
    if (ray_r_mu_intersects_ground) 
    {
        // Distance to the ground for the ray (r,mu), and its minimum and maximum
        // values over all mu - obtained for (r,-1) and (r,mu_horizon).
        float d = -r_mu - sqrt(max(0, discriminant));
        float d_min = r - atmosphere.bottom_radius;
        float d_max = rho;
        u_mu = 0.5 - 0.5 * GetTextureCoordFromUnitRange(d_max == d_min ? 0.0 :
            (d - d_min) / (d_max - d_min), SCATTERING_TEXTURE_MU_SIZE / 2);
    }
    else 
    {
        // Distance to the top atmosphere boundary for the ray (r,mu), and its
        // minimum and maximum values over all mu - obtained for (r,1) and
        // (r,mu_horizon).
        float d = -r_mu + sqrt(max(0, discriminant + H * H));
        float d_min = atmosphere.top_radius - r;
        float d_max = rho + H;
        u_mu = 0.5 + 0.5 * GetTextureCoordFromUnitRange(
            (d - d_min) / (d_max - d_min), SCATTERING_TEXTURE_MU_SIZE / 2);
    }

    float d = DistanceToTopAtmosphereBoundary(
        atmosphere, atmosphere.bottom_radius, mu_s);
    float d_min = atmosphere.top_radius - atmosphere.bottom_radius;
    float d_max = H;
    float a = (d - d_min) / (d_max - d_min);
    float D = DistanceToTopAtmosphereBoundary(
        atmosphere, atmosphere.bottom_radius, atmosphere.mu_s_min);
    float A = (D - d_min) / (d_max - d_min);
    // An ad-hoc function equal to 0 for mu_s = mu_s_min (because then d = D and
    // thus a = A), equal to 1 for mu_s = 1 (because then d = d_min and thus
    // a = 0), and with a large slope around mu_s = 0, to get more texture 
    // samples near the horizon.
    float u_mu_s = GetTextureCoordFromUnitRange(
        max(1.0 - a / A, 0.0) / (1.0 + a), SCATTERING_TEXTURE_MU_S_SIZE);

    float u_nu = (nu + 1.0) / 2.0;
    return float4(u_nu, u_mu_s, u_mu, u_r);
}

void GetRMuMuSNuFromScatteringTextureUvwz(in AtmosphereParameters atmosphere,
    in float4 uvwz, out float r, out float mu, out float mu_s,
    out float nu, out bool ray_r_mu_intersects_ground) 
{
    // Distance to top atmosphere boundary for a horizontal ray at ground level.
    float H = sqrt(atmosphere.top_radius * atmosphere.top_radius -
        atmosphere.bottom_radius * atmosphere.bottom_radius);
    // Distance to the horizon.
    float rho =
        H * GetUnitRangeFromTextureCoord(uvwz.w, SCATTERING_TEXTURE_R_SIZE);
    r = sqrt(rho * rho + atmosphere.bottom_radius * atmosphere.bottom_radius);

    if (uvwz.z < 0.5) 
    {
        // Distance to the ground for the ray (r,mu), and its minimum and maximum
        // values over all mu - obtained for (r,-1) and (r,mu_horizon) - from which
        // we can recover mu:
        float d_min = r - atmosphere.bottom_radius;
        float d_max = rho;
        float d = d_min + (d_max - d_min) * GetUnitRangeFromTextureCoord(
            1.0 - 2.0 * uvwz.z, SCATTERING_TEXTURE_MU_SIZE / 2);
        mu = d == 0.0 ? -1.0f :
            clamp((-(rho * rho + d * d) / (2.0 * r * d)), -1.0f, 1.0f);
        ray_r_mu_intersects_ground = true;
    }
    else 
    {
        // Distance to the top atmosphere boundary for the ray (r,mu), and its
        // minimum and maximum values over all mu - obtained for (r,1) and
        // (r,mu_horizon) - from which we can recover mu:
        float d_min = atmosphere.top_radius - r;
        float d_max = rho + H;
        float d = d_min + (d_max - d_min) * GetUnitRangeFromTextureCoord(
            2.0 * uvwz.z - 1.0, SCATTERING_TEXTURE_MU_SIZE / 2);
        mu = d == 0.0 ? 1.0f :
            clamp(((H * H - rho * rho - d * d) / (2.0 * r * d)), -1.0f, 1.0f);
        ray_r_mu_intersects_ground = false;
    }

    float x_mu_s =
        GetUnitRangeFromTextureCoord(uvwz.y, SCATTERING_TEXTURE_MU_S_SIZE);
    float d_min = atmosphere.top_radius - atmosphere.bottom_radius;
    float d_max = H;
    float D = DistanceToTopAtmosphereBoundary(
        atmosphere, atmosphere.bottom_radius, atmosphere.mu_s_min);
    float A = (D - d_min) / (d_max - d_min);
    float a = (A - x_mu_s * A) / (1.0 + x_mu_s * A);
    float d = d_min + min(a, A) * (d_max - d_min);
    mu_s = clamp((H * H - d * d) / (2.0f * atmosphere.bottom_radius * d), -1.0f, 1.0f);
    nu = clamp((uvwz.x * 2.0 - 1.0), -1.0f, 1.0f);
}

void GetRMuMuSNuFromScatteringTextureFragCoord(
    in AtmosphereParameters atmosphere, in float3 frag_coord,
    out float r, out float mu, out float mu_s, out float nu,
    out bool ray_r_mu_intersects_ground) 
{
    const float4 SCATTERING_TEXTURE_SIZE = float4(
        SCATTERING_TEXTURE_NU_SIZE - 1,
        SCATTERING_TEXTURE_MU_S_SIZE,
        SCATTERING_TEXTURE_MU_SIZE,
        SCATTERING_TEXTURE_R_SIZE);
    float frag_coord_nu =
        floor(frag_coord.x / float(SCATTERING_TEXTURE_MU_S_SIZE));
    float frag_coord_mu_s =
        frag_coord.x % float(SCATTERING_TEXTURE_MU_S_SIZE);
    float4 uvwz =
        float4(frag_coord_nu, frag_coord_mu_s, frag_coord.yz) /
            SCATTERING_TEXTURE_SIZE;
    GetRMuMuSNuFromScatteringTextureUvwz(
        atmosphere, uvwz, r, mu, mu_s, nu, ray_r_mu_intersects_ground);
    nu = clamp(nu, mu * mu_s - sqrt((1.0 - mu * mu) * (1.0 - mu_s * mu_s)),
        mu * mu_s + sqrt((1.0 - mu * mu) * (1.0 - mu_s * mu_s)));
}

/*
<p>With this mapping, we can finally write a function to precompute a texel of
the single scattering in a 3D texture:
*/

void ComputeSingleScatteringTexture(in AtmosphereParameters atmosphere,
    in Texture2D transmittance_texture,
    in SamplerState transmittance_sampler, 
    in float3 frag_coord,
    out float3 rayleigh, out float3 mie) 
{
    float r;
    float mu;
    float mu_s;
    float nu;
    bool ray_r_mu_intersects_ground;
    GetRMuMuSNuFromScatteringTextureFragCoord(atmosphere, frag_coord,
        r, mu, mu_s, nu, ray_r_mu_intersects_ground);
    ComputeSingleScattering(atmosphere, transmittance_texture, transmittance_sampler,
        r, mu, mu_s, nu, ray_r_mu_intersects_ground, rayleigh, mie);
}

float3 GetScattering(
    in AtmosphereParameters atmosphere,
    in Texture3D scattering_texture,
    in SamplerState scattering_sampler,
    float r, float mu, float mu_s, float nu,
    bool ray_r_mu_intersects_ground) 
{
    float4 uvwz = GetScatteringTextureUvwzFromRMuMuSNu(
        atmosphere, r, mu, mu_s, nu, ray_r_mu_intersects_ground);
    float tex_coord_x = uvwz.x * float(SCATTERING_TEXTURE_NU_SIZE - 1);
    float tex_x = floor(tex_coord_x);
    float t = tex_coord_x - tex_x;
    float3 uvw0 = float3((tex_x + uvwz.y) / SCATTERING_TEXTURE_NU_SIZE,
        uvwz.z, uvwz.w);
    float3 uvw1 = float3((tex_x + 1.0 + uvwz.y) / SCATTERING_TEXTURE_NU_SIZE,
        uvwz.z, uvwz.w);
    return lerp(scattering_texture.SampleLevel(scattering_sampler, uvw0, 0).xyz,
        scattering_texture.SampleLevel(scattering_sampler, uvw1, 0).xyz, t);
}

float3 GetScattering(
    in AtmosphereParameters atmosphere,
    in Texture3D single_rayleigh_scattering_texture,
    in Texture3D single_mie_scattering_texture,
    in Texture3D multiple_scattering_texture,
    in SamplerState linear_sampler,
    float r, float mu, float mu_s, float nu,
    bool ray_r_mu_intersects_ground,
    int scattering_order) {
    if (scattering_order == 1) {
    float3 rayleigh = GetScattering(
        atmosphere, single_rayleigh_scattering_texture, linear_sampler, r, mu, mu_s, nu,
        ray_r_mu_intersects_ground);
    float3 mie = GetScattering(
        atmosphere, single_mie_scattering_texture, linear_sampler, r, mu, mu_s, nu,
        ray_r_mu_intersects_ground);
    return rayleigh * RayleighPhaseFunction(nu) +
        mie * MiePhaseFunction(atmosphere.mie_phase_function_g, nu);
    }
    return GetScattering(
        atmosphere, multiple_scattering_texture, linear_sampler, r, mu, mu_s, nu,
        ray_r_mu_intersects_ground);
}

float3 GetIrradiance(
    in AtmosphereParameters atmosphere,
    in Texture2D irradiance_texture,
    in SamplerState irradiance_sampler,
    float r, float mu_s);

float3 ComputeScatteringDensity(
    in AtmosphereParameters atmosphere,
    in Texture2D transmittance_texture,
    in Texture3D single_rayleigh_scattering_texture,
    in Texture3D single_mie_scattering_texture,
    in Texture3D multiple_scattering_texture,
    in Texture2D irradiance_texture,
    in SamplerState linear_sampler,
    float r, float mu, float mu_s, float nu, int scattering_order) 
{
    // Compute unit direction vectors for the zenith, the view direction omega and
    // and the sun direction omega_s, such that the cosine of the view-zenith
    // angle is mu, the cosine of the sun-zenith angle is mu_s, and the cosine of
    // the view-sun angle is nu. The goal is to simplify computations below.
    float3 zenith_direction = float3(0.0, 0.0, 1.0);
    float3 omega = float3(sqrt(1.0 - mu * mu), 0.0, mu);
    float sun_dir_x = omega.x == 0.0 ? 0.0 : (nu - mu * mu_s) / omega.x;
    float sun_dir_y = sqrt(max(1.0 - sun_dir_x * sun_dir_x - mu_s * mu_s, 0.0));
    float3 omega_s = float3(sun_dir_x, sun_dir_y, mu_s);

    const float pi = 3.1415926535897932384626433832795f;
    const int SAMPLE_COUNT = 16;
    const float dphi = pi / float(SAMPLE_COUNT);
    const float dtheta = pi / float(SAMPLE_COUNT);
    float3 rayleigh_mie = 0.0f;

    // Nested loops for the integral over all the incident directions omega_i.
    for (int l = 0; l < SAMPLE_COUNT; ++l) 
    {
        float theta = (float(l) + 0.5) * dtheta;
        float cos_theta = cos(theta);
        float sin_theta = sin(theta);
        bool ray_r_theta_intersects_ground =
            RayIntersectsGround(atmosphere, r, cos_theta);

        // The distance and transmittance to the ground only depend on theta, so we
        // can compute them in the outer loop for efficiency.
        float distance_to_ground = 0.f;
        float3 transmittance_to_ground = 0.0f;
        float3 ground_albedo = 0.0f;
        if (ray_r_theta_intersects_ground)
        {
            distance_to_ground =
                DistanceToBottomAtmosphereBoundary(atmosphere, r, cos_theta);
            transmittance_to_ground =
                GetTransmittance(atmosphere, transmittance_texture, linear_sampler, r, cos_theta,
                    distance_to_ground, true /* ray_intersects_ground */);
            ground_albedo = atmosphere.ground_albedo;
        }

        for (int m = 0; m < 2 * SAMPLE_COUNT; ++m) 
        {
            float phi = (float(m) + 0.5) * dphi;
            float3 omega_i =
                float3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);
            float domega_i = dtheta * dphi * sin(theta);

            // The radiance L_i arriving from direction omega_i after n-1 bounces is
            // the sum of a term given by the precomputed scattering texture for the
            // (n-1)-th order:
            float nu1 = dot(omega_s, omega_i);
            float3 incident_radiance = GetScattering(atmosphere,
                single_rayleigh_scattering_texture, single_mie_scattering_texture,
                multiple_scattering_texture, linear_sampler, r, omega_i.z, mu_s, nu1,
                ray_r_theta_intersects_ground, scattering_order - 1);

            // and of the contribution from the light paths with n-1 bounces and whose
            // last bounce is on the ground. This contribution is the product of the
            // transmittance to the ground, the ground albedo, the ground BRDF, and
            // the irradiance received on the ground after n-2 bounces.
            float3 ground_normal =
                normalize(zenith_direction * r + omega_i * distance_to_ground);
            float3 ground_irradiance = GetIrradiance(
                atmosphere, irradiance_texture, linear_sampler, atmosphere.bottom_radius,
                dot(ground_normal, omega_s));
            incident_radiance += transmittance_to_ground *
                ground_albedo * (1.0 / pi) * ground_irradiance;

            // The radiance finally scattered from direction omega_i towards direction
            // -omega is the product of the incident radiance, the scattering
            // coefficient, and the phase function for directions omega and omega_i
            // (all this summed over all particle types, i.e. Rayleigh and Mie).
            float nu2 = dot(omega, omega_i);
            float rayleigh_density = GetProfileDensity(
                atmosphere.rayleigh_density, r - atmosphere.bottom_radius);
            float mie_density = GetProfileDensity(
                atmosphere.mie_density, r - atmosphere.bottom_radius);
            rayleigh_mie += incident_radiance * (
                atmosphere.rayleigh_scattering * rayleigh_density *
                    RayleighPhaseFunction(nu2) +
                atmosphere.mie_scattering * mie_density *
                    MiePhaseFunction(atmosphere.mie_phase_function_g, nu2)) *
                domega_i;
        }
    }
    return rayleigh_mie;
}

float3 ComputeMultipleScattering(
    in AtmosphereParameters atmosphere,
    in Texture2D transmittance_texture,
    in Texture3D scattering_density_texture,
    in SamplerState linear_sampler,
    float r, float mu, float mu_s, float nu,
    bool ray_r_mu_intersects_ground) 
{
    // Number of intervals for the numerical integration.
    const int SAMPLE_COUNT = 50;
    // The integration step, i.e. the length of each integration interval.
    float dx =
        DistanceToNearestAtmosphereBoundary(
            atmosphere, r, mu, ray_r_mu_intersects_ground) /
                float(SAMPLE_COUNT);
    // Integration loop.
    float3 rayleigh_mie_sum = 0;
    for (int i = 0; i <= SAMPLE_COUNT; ++i) 
    {
        float d_i = float(i) * dx;

        // The r, mu and mu_s parameters at the current integration point (see the
        // single scattering section for a detailed explanation).
        float r_i = clamp(sqrt(d_i * d_i + 2.0 * r * mu * d_i + r * r), atmosphere.bottom_radius, atmosphere.top_radius);
        float mu_i = clamp((r * mu + d_i) / r_i, -1.0f, 1.0f);
        float mu_s_i = clamp((r * mu_s + d_i * nu) / r_i, -1.0f, 1.0f);

        // The Rayleigh and Mie multiple scattering at the current sample point.
        float3 rayleigh_mie_i =
            GetScattering(
                atmosphere, scattering_density_texture, linear_sampler, r_i, mu_i, mu_s_i, nu,
                ray_r_mu_intersects_ground) *
            GetTransmittance(
                atmosphere, transmittance_texture, linear_sampler, r, mu, d_i,
                ray_r_mu_intersects_ground) *
            dx;
        // Sample weight (from the trapezoidal rule).
        float weight_i = (i == 0 || i == SAMPLE_COUNT) ? 0.5 : 1.0;
        rayleigh_mie_sum += rayleigh_mie_i * weight_i;
    }
    return rayleigh_mie_sum;
}

float3 ComputeScatteringDensityTexture(
    in AtmosphereParameters atmosphere,
    in Texture2D transmittance_texture,
    in Texture3D single_rayleigh_scattering_texture,
    in Texture3D single_mie_scattering_texture,
    in Texture3D multiple_scattering_texture,
    in Texture2D irradiance_texture,
    in SamplerState linear_sampler,
    in float3 frag_coord, int scattering_order) 
{
    float r;
    float mu;
    float mu_s;
    float nu;
    bool ray_r_mu_intersects_ground;
    GetRMuMuSNuFromScatteringTextureFragCoord(atmosphere, frag_coord,
        r, mu, mu_s, nu, ray_r_mu_intersects_ground);
    return ComputeScatteringDensity(atmosphere, transmittance_texture,
        single_rayleigh_scattering_texture, single_mie_scattering_texture,
        multiple_scattering_texture, irradiance_texture, linear_sampler, r, mu, mu_s, nu,
        scattering_order);
}

float3 ComputeMultipleScatteringTexture(
    in AtmosphereParameters atmosphere,
    in Texture2D transmittance_texture,
    in Texture3D scattering_density_texture,
    in SamplerState linear_sampler,
    in float3 frag_coord, out float nu) 
{
    float r;
    float mu;
    float mu_s;
    bool ray_r_mu_intersects_ground;
    GetRMuMuSNuFromScatteringTextureFragCoord(atmosphere, frag_coord,
        r, mu, mu_s, nu, ray_r_mu_intersects_ground);
    return ComputeMultipleScattering(atmosphere, transmittance_texture,
        scattering_density_texture, linear_sampler, r, mu, mu_s, nu,
        ray_r_mu_intersects_ground);
}

float3 ComputeDirectIrradiance(
    in AtmosphereParameters atmosphere,
    in Texture2D transmittance_texture,
    in SamplerState linear_sampler,
    float r, float mu_s) 
{
    float alpha_s = atmosphere.sun_angular_radius;
    // Approximate average of the cosine factor mu_s over the visible fraction of
    // the Sun disc.
    float average_cosine_factor =
        mu_s < -alpha_s ? 0.0 : (mu_s > alpha_s ? mu_s :
        (mu_s + alpha_s) * (mu_s + alpha_s) / (4.0 * alpha_s));

    return /*atmosphere.solar_irradiance **/
        GetTransmittanceToTopAtmosphereBoundary(
            atmosphere, transmittance_texture, linear_sampler, r, mu_s) * average_cosine_factor;
}

float3 ComputeIndirectIrradiance(
    in AtmosphereParameters atmosphere,
    in Texture3D single_rayleigh_scattering_texture,
    in Texture3D single_mie_scattering_texture,
    in Texture3D multiple_scattering_texture,
    in SamplerState linear_sampler,
    float r, float mu_s, int scattering_order) 
{
    const float pi = 3.1415926535897932384626433832795f;
    const int SAMPLE_COUNT = 32;
    const float dphi = pi / float(SAMPLE_COUNT);
    const float dtheta = pi / float(SAMPLE_COUNT);

    float3 result = 0.0f;
    float3 omega_s = float3(sqrt(1.0 - mu_s * mu_s), 0.0, mu_s);
    for (int j = 0; j < SAMPLE_COUNT / 2; ++j) 
    {
        float theta = (float(j) + 0.5) * dtheta;
        for (int i = 0; i < 2 * SAMPLE_COUNT; ++i) 
        {
            float phi = (float(i) + 0.5) * dphi;
            float3 omega =
                float3(cos(phi) * sin(theta), sin(phi) * sin(theta), cos(theta));
            float domega = dtheta * dphi * sin(theta);

            float nu = dot(omega, omega_s);
            // f(omega_i) = multi_scattered_irradiance * cos(omega_i) * delta_omega
            result += GetScattering(atmosphere, single_rayleigh_scattering_texture,
                single_mie_scattering_texture, multiple_scattering_texture, linear_sampler, 
                r, omega.z, mu_s, nu, false /* ray_r_theta_intersects_ground */,
                scattering_order) *
                    omega.z * domega;
        }
    }
    return result;
}

float2 GetIrradianceTextureUvFromRMuS(in AtmosphereParameters atmosphere,
    float r, float mu_s) 
{
    float x_r = (r - atmosphere.bottom_radius) /
        (atmosphere.top_radius - atmosphere.bottom_radius);
    float x_mu_s = mu_s * 0.5 + 0.5;
    return float2(GetTextureCoordFromUnitRange(x_mu_s, IRRADIANCE_TEXTURE_WIDTH),
                GetTextureCoordFromUnitRange(x_r, IRRADIANCE_TEXTURE_HEIGHT));
}

void GetRMuSFromIrradianceTextureUv(in AtmosphereParameters atmosphere,
    in float2 uv, out float r, out float mu_s) 
{
    float x_mu_s = GetUnitRangeFromTextureCoord(uv.x, IRRADIANCE_TEXTURE_WIDTH);
    float x_r = GetUnitRangeFromTextureCoord(uv.y, IRRADIANCE_TEXTURE_HEIGHT);
    r = atmosphere.bottom_radius +
        x_r * (atmosphere.top_radius - atmosphere.bottom_radius);
    mu_s = clamp(2.0 * x_mu_s - 1.0, -1.0f, 1.0f);
}

float3 ComputeDirectIrradianceTexture(
    in AtmosphereParameters atmosphere,
    in Texture2D transmittance_texture,
    in SamplerState linear_sampler,
    in float2 frag_coord) 
{
    float r;
    float mu_s;
    GetRMuSFromIrradianceTextureUv(
        atmosphere, frag_coord / IRRADIANCE_TEXTURE_SIZE, r, mu_s);
    return ComputeDirectIrradiance(atmosphere, transmittance_texture, linear_sampler, r, mu_s);
}

float3 ComputeIndirectIrradianceTexture(
    in AtmosphereParameters atmosphere,
    in Texture3D single_rayleigh_scattering_texture,
    in Texture3D single_mie_scattering_texture,
    in Texture3D multiple_scattering_texture,
    in SamplerState linear_sampler,
    in float2 frag_coord, int scattering_order) 
{
    float r;
    float mu_s;
    GetRMuSFromIrradianceTextureUv(
        atmosphere, frag_coord / IRRADIANCE_TEXTURE_SIZE, r, mu_s);
    return ComputeIndirectIrradiance(atmosphere,
        single_rayleigh_scattering_texture, single_mie_scattering_texture,
        multiple_scattering_texture, linear_sampler, r, mu_s, scattering_order);
}

float3 GetIrradiance(
    in AtmosphereParameters atmosphere,
    in Texture2D irradiance_texture,
    in SamplerState linear_sampler,
    float r, float mu_s) 
{
    float2 uv = GetIrradianceTextureUvFromRMuS(atmosphere, r, mu_s);
    return irradiance_texture.SampleLevel(linear_sampler, uv, 0).xyz;
}

#ifdef COMBINED_SCATTERING_TEXTURES
float3 GetExtrapolatedSingleMieScattering(
    in AtmosphereParameters atmosphere, in float4 scattering) 
{
    // Algebraically this can never be negative, but rounding errors can produce
    // that effect for sufficiently short view rays.
    return scattering.r <= 0.0 ? 0.0f : scattering.rgb * scattering.a / scattering.r *
        (atmosphere.rayleigh_scattering.r / atmosphere.mie_scattering.r) *
        (atmosphere.mie_scattering / atmosphere.rayleigh_scattering);
}
#endif

float3 GetCombinedScattering(
    in AtmosphereParameters atmosphere,
    in Texture3D scattering_texture,
    in Texture3D single_mie_scattering_texture,
    in SamplerState linear_sampler,
    float r, float mu, float mu_s, float nu,
    bool ray_r_mu_intersects_ground,
    out float3 single_mie_scattering) 
{
    float4 uvwz = GetScatteringTextureUvwzFromRMuMuSNu(
        atmosphere, r, mu, mu_s, nu, ray_r_mu_intersects_ground);
    float tex_coord_x = uvwz.x * float(SCATTERING_TEXTURE_NU_SIZE - 1);
    float tex_x = floor(tex_coord_x);
    float t = tex_coord_x - tex_x;
    float3 uvw0 = float3((tex_x + uvwz.y) / float(SCATTERING_TEXTURE_NU_SIZE),
        uvwz.z, uvwz.w);
    float3 uvw1 = float3((tex_x + 1.0 + uvwz.y) / float(SCATTERING_TEXTURE_NU_SIZE),
        uvwz.z, uvwz.w);
#ifdef COMBINED_SCATTERING_TEXTURES
    float4 combined_scattering =
        lerp(scattering_texture.SampleLevel(linear_sampler, uvw0, 0), scattering_texture.SampleLevel(linear_sampler, uvw1, 0), t);
    float3 scattering = combined_scattering.xyz;
    single_mie_scattering =
        GetExtrapolatedSingleMieScattering(atmosphere, combined_scattering);
#else
    float3 scattering = 
        lerp(scattering_texture.SampleLevel(linear_sampler, uvw0, 0), scattering_texture.SampleLevel(linear_sampler, uvw1, 0), t).xyz;
    single_mie_scattering = 
        lerp(single_mie_scattering_texture.SampleLevel(linear_sampler, uvw0, 0), single_mie_scattering_texture.SampleLevel(linear_sampler, uvw1, 0), t).xyz;
#endif
    return scattering;  
}

float3 GetSkyRadiance(
    in AtmosphereParameters atmosphere,
    in Texture2D transmittance_texture,
    in Texture3D scattering_texture,
    in Texture3D single_mie_scattering_texture,
    in SamplerState linear_sampler,
    in float3 camera, in float3 view_ray, float shadow_length,
    in float3 sun_direction, out float3 transmittance) 
{
    // Compute the distance to the top atmosphere boundary along the view ray,
    // assuming the viewer is in space (or NaN if the view ray does not intersect
    // the atmosphere).
    float r = length(camera);
    float rmu = dot(camera, view_ray);
    // r * r + (-rmu + x) * (-rmu + x) + 2 * rmu * (x - rmu)  = R * R
    // x2 = R2 + rmu2 - r2
    float distance_to_top_atmosphere_boundary = -rmu -
        sqrt(rmu * rmu - r * r + atmosphere.top_radius * atmosphere.top_radius);
    // If the viewer is in space and the view ray intersects the atmosphere, move
    // the viewer to the top atmosphere boundary (along the view ray):
    if (distance_to_top_atmosphere_boundary > 0.0f) 
    {
        camera = camera + view_ray * distance_to_top_atmosphere_boundary;
        r = atmosphere.top_radius;
        rmu += distance_to_top_atmosphere_boundary;
    }
    else if (r > atmosphere.top_radius) 
    {
        // If the view ray does not intersect the atmosphere, simply return 0.
        transmittance = 1.0f;
        return 0.0f;
    }
    // Compute the r, mu, mu_s and nu parameters needed for the texture lookups.
    float mu = rmu / r;
    float mu_s = dot(camera, sun_direction) / r;
    float nu = dot(view_ray, sun_direction);
    bool ray_r_mu_intersects_ground = RayIntersectsGround(atmosphere, r, mu);

    transmittance = ray_r_mu_intersects_ground ? 0.0f :
        GetTransmittanceToTopAtmosphereBoundary(
            atmosphere, transmittance_texture, linear_sampler, r, mu);
    float3 single_mie_scattering;
    float3 scattering;
    if (shadow_length == 0.0f) 
    {
        scattering = GetCombinedScattering(
            atmosphere, scattering_texture, single_mie_scattering_texture, linear_sampler,
            r, mu, mu_s, nu, ray_r_mu_intersects_ground,
            single_mie_scattering);
    }
    else 
    {
        // Case of light shafts (shadow_length is the total length noted l in our
        // paper): we omit the scattering between the camera and the point at
        // distance l, by implementing Eq. (18) of the paper (shadow_transmittance
        // is the T(x,x_s) term, scattering is the S|x_s=x+lv term).
        float d = shadow_length;
        float r_p =
            clamp(sqrt(d * d + 2.0 * r * mu * d + r * r), atmosphere.bottom_radius, atmosphere.top_radius);
        float mu_p = (r * mu + d) / r_p;
        float mu_s_p = (r * mu_s + d * nu) / r_p;

        scattering = GetCombinedScattering(
            atmosphere, scattering_texture, single_mie_scattering_texture, linear_sampler,
            r_p, mu_p, mu_s_p, nu, ray_r_mu_intersects_ground, 
            single_mie_scattering);
        float3 shadow_transmittance =
            GetTransmittance(atmosphere, transmittance_texture, linear_sampler,
                r, mu, shadow_length, ray_r_mu_intersects_ground);
        scattering = scattering * shadow_transmittance;
        single_mie_scattering = single_mie_scattering * shadow_transmittance;
    }
    return scattering * RayleighPhaseFunction(nu) + single_mie_scattering *
        MiePhaseFunction(atmosphere.mie_phase_function_g, nu);
}

float3 GetSkyRadianceToPoint(
    AtmosphereParameters atmosphere,
    Texture2D transmittance_texture,
    Texture3D scattering_texture,
    Texture3D single_mie_scattering_texture,
    SamplerState linear_sampler,
    float3 camera, float3 p, float shadow_length,
    float3 sun_direction, out float3 transmittance) 
{
    // Compute the distance to the top atmosphere boundary along the view ray,
    // assuming the viewer is in space (or NaN if the view ray does not intersect
    // the atmosphere).
    float3 view_ray = normalize(p - camera);
    float r = length(camera);
    float rmu = dot(camera, view_ray);
    float distance_to_top_atmosphere_boundary = -rmu -
        sqrt(rmu * rmu - r * r + atmosphere.top_radius * atmosphere.top_radius);
    // If the viewer is in space and the view ray intersects the atmosphere, move
    // the viewer to the top atmosphere boundary (along the view ray):
    if (distance_to_top_atmosphere_boundary > 0.0) 
    {
        camera = camera + view_ray * distance_to_top_atmosphere_boundary;
        r = atmosphere.top_radius;
        rmu += distance_to_top_atmosphere_boundary;
    }

    // Compute the r, mu, mu_s and nu parameters for the first texture lookup.
    float mu = rmu / r;
    float mu_s = dot(camera, sun_direction) / r;
    float nu = dot(view_ray, sun_direction);
    float d = length(p - camera);
    bool ray_r_mu_intersects_ground = RayIntersectsGround(atmosphere, r, mu);

    transmittance = GetTransmittance(atmosphere, transmittance_texture, linear_sampler,
        r, mu, d, ray_r_mu_intersects_ground);

    float3 single_mie_scattering;
    float3 scattering = GetCombinedScattering(
        atmosphere, scattering_texture, single_mie_scattering_texture, linear_sampler,
        r, mu, mu_s, nu, ray_r_mu_intersects_ground,
        single_mie_scattering);

    // Compute the r, mu, mu_s and nu parameters for the second texture lookup.
    // If shadow_length is not 0 (case of light shafts), we want to ignore the
    // scattering along the last shadow_length meters of the view ray, which we
    // do by subtracting shadow_length from d (this way scattering_p is equal to
    // the S|x_s=x_0-lv term in Eq. (17) of our paper).
    d = max(d - shadow_length, 0.0);
    float r_p = clamp(sqrt(d * d + 2.0 * r * mu * d + r * r), atmosphere.bottom_radius, atmosphere.top_radius);
    float mu_p = (r * mu + d) / r_p;
    float mu_s_p = (r * mu_s + d * nu) / r_p;

    float3 single_mie_scattering_p;
    float3 scattering_p = GetCombinedScattering(
        atmosphere, scattering_texture, single_mie_scattering_texture, linear_sampler,
        r_p, mu_p, mu_s_p, nu, ray_r_mu_intersects_ground,
        single_mie_scattering_p);

    // Combine the lookup results to get the scattering between camera and point.
    float3 shadow_transmittance = transmittance;
    if (shadow_length > 0.0) 
    {
        // This is the T(x,x_s) term in Eq. (17) of our paper, for light shafts.
        shadow_transmittance = GetTransmittance(atmosphere, transmittance_texture, linear_sampler,
            r, mu, d, ray_r_mu_intersects_ground);
    }
    scattering = scattering - shadow_transmittance * scattering_p;
    single_mie_scattering =
        single_mie_scattering - shadow_transmittance * single_mie_scattering_p;
    #ifdef COMBINED_SCATTERING_TEXTURES
    single_mie_scattering = GetExtrapolatedSingleMieScattering(
        atmosphere, float4(scattering, single_mie_scattering.r));
    #endif

    // Hack to avoid rendering artifacts when the sun is below the horizon.
    single_mie_scattering = single_mie_scattering *
        smoothstep(0.0f, 0.01f, mu_s);

    return scattering * RayleighPhaseFunction(nu) + single_mie_scattering *
        MiePhaseFunction(atmosphere.mie_phase_function_g, nu);
}

float3 GetSunAndSkyIrradiance(
    AtmosphereParameters atmosphere,
    Texture2D transmittance_texture,
    Texture2D irradiance_texture,
    SamplerState linear_sampler,
    float3 p, float3 normal, float3 sun_direction,
    out float3 sky_irradiance) 
{
    float r = length(p);
    float mu_s = dot(p, sun_direction) / r;

    // Indirect irradiance (approximated if the surface is not horizontal).
    sky_irradiance = GetIrradiance(atmosphere, irradiance_texture, linear_sampler, r, mu_s) *
        (1.0 + dot(normal, p) / r) * 0.5;

    // Direct irradiance.
    return /*atmosphere.solar_irradiance **/
        GetTransmittanceToSun(
            atmosphere, transmittance_texture, linear_sampler, r, mu_s) *
        max(dot(normal, sun_direction), 0.0);
}