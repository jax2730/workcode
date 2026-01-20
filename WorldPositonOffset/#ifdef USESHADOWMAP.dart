    #ifdef USESHADOWMAP
 		    vec4 wpos = vec4(WorldP, 1.0f);
            vec3 wNormal = normalize(WorldN.xyz);
			float xx = dot(wNormal.xyz, -U_VS_MainLightDirection.xyz);
			xx = max(0.0, xx);
			xx = pow(xx, 5.0f);
			xx = 1.0 - xx;
			wpos.xyz = wpos.xyz + wNormal.xyz * 0.008 * xx;
			
			#if SHADOWSPLITE > 0
				LightSpacePos[0].x = dot(U_LightVPMatrix0[0], wpos);
				LightSpacePos[0].y = dot(U_LightVPMatrix0[1], wpos);
				LightSpacePos[0].z = dot(U_LightVPMatrix0[2], wpos);
				LightSpacePos[0].w = dot(U_LightVPMatrix0[3], wpos);
				LightSpacePos[0] = LightSpacePos[0] / LightSpacePos[0].w;
				LightSpacePos[0].xyz = LightSpacePos[0].xyz * 0.5 + 0.5;
			#endif // SHADOWSPLITE > 0
			#if SHADOWSPLITE > 1	
				LightSpacePos[1].x = dot(U_LightVPMatrix1[0], wpos);
				LightSpacePos[1].y = dot(U_LightVPMatrix1[1], wpos);
				LightSpacePos[1].z = dot(U_LightVPMatrix1[2], wpos);
				LightSpacePos[1].w = dot(U_LightVPMatrix1[3], wpos);
				LightSpacePos[1] = LightSpacePos[1] / LightSpacePos[1].w;
				LightSpacePos[1].xyz = LightSpacePos[1].xyz * 0.5 + 0.5;
			#endif // SHADOWSPLITE > 1
			#if SHADOWSPLITE > 2	
				LightSpacePos[2].x = dot(U_LightVPMatrix2[0], wpos);
				LightSpacePos[2].y = dot(U_LightVPMatrix2[1], wpos);
				LightSpacePos[2].z = dot(U_LightVPMatrix2[2], wpos);
				LightSpacePos[2].w = dot(U_LightVPMatrix2[3], wpos);
				LightSpacePos[2] = LightSpacePos[2] / LightSpacePos[2].w;
				LightSpacePos[2].xyz = LightSpacePos[2].xyz * 0.5 + 0.5;
			#endif // SHADOWSPLITE > 2			
			ViewSpaceZ = dot(U_MainCamViewMatrix, wpos);
        #endif // USESHADOWMAP
