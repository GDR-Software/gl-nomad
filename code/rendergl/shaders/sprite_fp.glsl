#if !defined(GLSL_LEGACY)
layout( location = 0 ) out vec4 a_Color;
layout( location = 1 ) out vec4 a_BrightColor;
#endif

in vec2 v_TexCoords;
in vec4 v_Color;
in vec3 v_WorldPos;
in vec3 v_Position;

uniform float u_GammaAmount;
uniform bool u_GamePaused;
uniform float u_CameraExposure;

uniform mat4 u_ModelViewProjection;
uniform bool u_HardwareGamma;
uniform bool u_HDR;
uniform bool u_PBR;
uniform int u_AntiAliasing;
uniform int u_AntiAliasingQuality;
uniform int u_LightingQuality;
uniform bool u_Bloom;
uniform bool u_PostProcess;
uniform int u_AlphaTest;

TEXTURE2D u_DiffuseMap;
uniform vec3 u_ViewOrigin;
uniform vec4 u_SpecularScale;
uniform vec4 u_NormalScale;

#if defined(USE_NORMALMAP)
TEXTURE2D u_NormalMap;
#endif

#if defined(USE_SPECULARMAP)
TEXTURE2D u_SpecularMap;
#endif

struct Light {
	vec4 color;
	uvec2 origin;
	float brightness;
	float range;
	float linear;
	float quadratic;
	float constant;
	int type;
};

layout( std140, binding = 0 ) buffer u_LightBuffer {
	Light u_LightData[];
};

uniform int u_NumLights;
uniform vec3 u_AmbientColor;

#include "image_sharpen.glsl"
#include "fxaa.glsl"

float CalcLightAttenuation(float point, float normDist)
{
	// zero light at 1.0, approximating q3 style
	// also don't attenuate directional light
	float attenuation = (0.5 * normDist - 1.5) * point + 1.0;

	// clamp attenuation
	#if defined(NO_LIGHT_CLAMP)
	attenuation = max(attenuation, 0.0);
	#else
	attenuation = clamp(attenuation, 0.0, 1.0);
	#endif

	return attenuation;
}

vec3 CalcDiffuse( vec3 diffuseAlbedo, float NH, float EH, float roughness )
{
#if defined(USE_BURLEY)
	// modified from https://disney-animation.s3.amazonaws.com/library/s2012_pbs_disney_brdf_notes_v2.pdf
	float fd90 = -0.5 + EH * EH * roughness;
	float burley = 1.0 + fd90 * 0.04 / NH;
	burley *= burley;
	return diffuseAlbedo * burley;
#else
	return diffuseAlbedo;
#endif
}

vec3 CalcNormal() {
#if defined(USE_NORMALMAP)
	vec3 normal = texture2D( u_NormalMap, v_TexCoords ).rgb;
	normal = normal * 2.0 - 1.0;
	return normalize( normal );
#else
	return vec3( 0.0 );
#endif
}

vec3 CalcScreenSpaceNormal( vec3 position ) {
	vec3 dx = dFdx( position );
	vec3 dy = dFdy( position );
	return normalize( cross( dx, dy ) );
}

vec3 CalcPointLight( Light light, bool isDLight ) {
	vec3 diffuse = a_Color.rgb;
	float dist = distance( v_WorldPos, vec3( light.origin, gl_FragCoord.z ) );
	float diff = 0.0;
	float range = light.range;
	vec3 specular = vec3( 0.0 );
	float attenuation = 1.0;

	if ( dist <= light.range ) {
		diff = 1.0 - abs( dist / light.range );
	}
	diff += light.brightness;
	diffuse = min( diff * ( diffuse + vec3( light.color ) ), diffuse );

	range = light.range * light.brightness;

	attenuation = ( light.constant + light.linear + light.quadratic * ( light.range * light.range ) );
	if ( u_LightingQuality == 2 ) {
		vec3 lightDir = vec3( light.origin.xy, 0.0 ) - gl_FragCoord.xyz;
		vec3 viewDir = normalize( u_ViewOrigin - v_WorldPos );
		vec3 halfwayDir = normalize( lightDir + viewDir );

		vec3 reflectDir = reflect( -lightDir, v_WorldPos );
	#if defined(USE_SPECULARMAP)
		float shininess = texture2D( u_SpecularMap, v_TexCoords ).r;
	#else
		float shininess = 16.0;
	#endif
		if ( shininess < 16.0 ) {
			// anything lower than this is barely visible
			shininess = 16.0;
		}

		vec3 normal = CalcNormal();
		if ( normal == vec3( 1.0 ) ) {
			normal = vec3( 0.0 );
		}
		const float energyConservation = ( 8.0 + shininess ) / ( 8.0 * M_PI );
		float spec = energyConservation * pow( max( dot( normal, halfwayDir ), 0.0 ), shininess );
		specular = light.color.rgb * vec3( spec );
		specular *= attenuation;

		diffuse = mix( diffuse, normal, 0.025 );
	}
	else if ( u_LightingQuality == 1 ) {
		const vec3 normal = CalcNormal();
		const vec3 lightDir = normalize( v_WorldPos ) - normalize( vec3( light.origin.xy, 0.0 ) );
		diffuse = mix( diffuse, normal, 0.025 );
	}

	diffuse *= attenuation;

	return diffuse + specular;
}

void ApplyLighting() {
	for ( int i = 0; i < u_NumLights; i++ ) {
		switch ( u_LightData[i].type ) {
		case POINT_LIGHT:
			a_Color.rgb += CalcPointLight( u_LightData[i], i > MAX_MAP_LIGHTS );
			break;
		case DIRECTION_LIGHT:
			break;
		};
	}
	a_Color.rgb *= u_AmbientColor;
}

void main() {
	if ( distance( u_ViewOrigin, v_WorldPos ) > 12.0 ) {
		discard;
	}

	// calculate a slight x offset, otherwise we get some black line bleeding
	// going on
	ivec2 texSize = textureSize( u_DiffuseMap, 0 );
	float sOffset = ( 1.0 / ( float( texSize.x ) ) * 0.75 );
	float tOffset = ( 1.0 / ( float( texSize.y ) ) * 0.75 );
	vec2 texCoord = vec2( v_TexCoords.x + sOffset, v_TexCoords.y + tOffset );

	if ( !u_PostProcess ) {
		if ( u_AntiAliasing == AntiAlias_FXAA ) {
			vec2 fragCoord = texCoord * u_ScreenSize;
			a_Color = ApplyFXAA( u_DiffuseMap, fragCoord );
		} else {
			a_Color = sharpenImage( u_DiffuseMap, texCoord );
		}
	} else {
		a_Color = texture2D( u_DiffuseMap, texCoord );
	}

	const float alpha = a_Color.a * v_Color.a;
	if ( u_AlphaTest == 1 ) {
		if ( alpha == 0.0 ) {
			discard;
		}
	}
	else if ( u_AlphaTest == 2 ) {
		if ( alpha >= 0.5 ) {
			discard;
		}
	}
	else if ( u_AlphaTest == 3 ) {
		if ( alpha < 0.5 ) {
			discard;
		}
	}
	a_Color.a = alpha;

	ApplyLighting();

	// if we have post processing active, don't calculate gamma until the final pass
	if ( u_HDR ) {
		if ( u_Bloom ) {
			// check whether fragment output is higher than threshold, if so output as brightness color
			float brightness = dot( a_Color.rgb, vec3( 0.1, 0.1, 0.1 ) );
			if ( brightness > 0.5 ) {
				a_BrightColor = vec4( a_Color.rgb, 1.0 );
			} else {
				a_BrightColor = vec4( 0.0, 0.0, 0.0, 1.0 );
			}
		}
	}
	if ( u_GamePaused ) {
		a_Color.rgb = vec3( a_Color.rg * 0.5, 0.5 );
	}
}