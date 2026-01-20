//-----------------------------------------------------------------------------
// File:   EchoTODManager.cpp
//
// Author: chenanzhi
//
// Date:   2016-6-23
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#include "EchoStableHeaders.h"
#include "EchoTODManager.h"
#include "EchoTODSerializer.h"
#include "EchoSceneManager.h"
#include "EchoRoot.h"
#include "EchoSky.h"
#include "EchoStars.h"
#include "EchoCloud.h"
#include "EchoSun.h"
#include "EchoOcean.h"
#include "EchoSphericalOcean.h"
#include "EchoVolumetricLight.h"
#include "EchoLensFlare.h"
//#include "EchoAreaManager.h"
//#include "EchoSceneArea.h"
#include "EchoRenderStrategy.h"
#include "EchoPostProcess.h"
#include "EchoColorGrading.h"
#include "EchoBloom.h"
#include "EchoHDR.h"
#include "EchoEnvironmentLight.h"
#include "EchoProtocolComponent.h"
#include "EchoSphericalTerrainAtmosphere.h"

#ifndef _WIN32
#include <sys/time.h>
#endif
#include "EchoTextureManager.h"

//#pragma warning(disable:4244)
//-----------------------------------------------------------------------------
//	CPP File Begin
//-----------------------------------------------------------------------------

namespace Echo
{
	static const String TOD_PATTERN = "*.tod";
	static const String TOD_SUFFIX = ".tod";
	static const double g_dayFactor = 0.291667; //7点
	static const double g_nightFactor = 0.791667; //19点

	TODManager::TODManager()
		: mCurrentVariableInfo(nullptr)
		, mOldVariableInfo(nullptr)
		, mCurrentTime(0.5)
		, mMainLightPitchLimit(-10.0f)
		, mLastUpdateTime(0.0)
		, mPlayEnable(true)
		, mStaticTime(0.5)
		, mSceneManager(nullptr)
		, mTODPriorityLevel(e_envAreaLvl)
		, mMoonRise(false)
		, mElapseTime(1.0f)
		, mTranslationTime(1.0f)
		, mSrcPlayEnable(false)
		, mSrcStaticTime(0.5)
		, m_dwSvrTime(0)
		, m_dwTickTime(0)
		, m_dwDayTime(24 * 3600)
		, m_dwUpdateTick(0)
		, m_currentTotalTime(0.0)
		, m_currentRealTotalTime(0.0)
		, mPreNeedUpdateWeatherParam(false)
		, mForceUpdate(false)
		, mUseCustomColorChart(false)
		, mCurrentColorChartName("Default")
	{
		EchoZoneScoped;

		setTimeStep(1);
		setTimeSpeed(1);

		String defaulttodname = "DefaultTOD.tod";
		mCommonTodName = defaulttodname;
		mRainTodName = "Rain.tod";

		mRootPath = Root::instance()->getRootResourcePath();
		mTodFileDir.append(mRootPath);
		mTodFileDir.append("tod/");

		Root::instance()->addFrameListener(this);


		if (Root::instance()->getRuntimeMode() == Root::eRM_Editor)
			mIsUseRainState = true;
	}

	TODManager::~TODManager(void)
	{
		EchoZoneScoped;

		Root::instance()->removeFrameListener(this);
		cleanup();
		mSceneManager = nullptr;
	}

	String TODManager::createUniqueID(const String& name)
	{
		EchoZoneScoped;

		//String newName = name;
		//String::size_type dotpos = name.find_last_of(".");
		//if (dotpos > 0)
		//{
		//	newName = name.substr(0, dotpos);
		//}
		///@todo		
		return name;
	}

	TODVariableInfo* TODManager::prepareTod(const String& name)
	{
		EchoZoneScoped;

		if (name.empty())
			return nullptr;

		TODMap::iterator itfind = mTodMap.find(name);
		if (itfind != mTodMap.end())
			return itfind->second;

		TODVariableInfo* pVariableInfo = new TODVariableInfo(this);
		bool rst = TODSerializer::loadTodFile(name, pVariableInfo);
		if (!rst)
		{
			delete pVariableInfo;
			return nullptr;
		}

		prepareTexture(pVariableInfo);

		mTodMap[name] = pVariableInfo;

		for (TODManagerListenners::iterator it = mListenners.begin(); it != mListenners.end(); ++it)
		{
			TODManagerListenner* l = *it;
			if (l)
			{
				l->onTodListChanged(mTodMap);
			}
		}

		return pVariableInfo;
	}

	TODVariableInfo* TODManager::loadTod(const String& name)
	{
		EchoZoneScoped;

		if (name.empty())
			return nullptr;

		TODVariableInfo* pVariableInfo = nullptr;
		pVariableInfo = prepareTod(name);

		if (pVariableInfo && !pVariableInfo->getTextureLoaded())
		{
			loadTexture(pVariableInfo);
		}

		return pVariableInfo;
	}

	void TODManager::removeTod(const String& name)
	{
		EchoZoneScoped;

		TODMap::iterator it = mTodMap.find(name);
		if (it != mTodMap.end())
		{
			TODVariableInfo* tm = it->second;
			if (mCurrentVariableInfo == tm)
			{
				mCurrentVariableInfo = nullptr;
				for (TODManagerListenners::iterator itor = mListenners.begin(); itor != mListenners.end(); ++itor)
				{
					TODManagerListenner* l = *itor;
					if (l)
						l->onTodRemove();
				}
			}
			if (mOldVariableInfo == tm)
			{
				mOldVariableInfo = nullptr;
			}

			delete tm;
			mTodMap.erase(it);

			for (TODManagerListenners::iterator itor = mListenners.begin(); itor != mListenners.end(); ++itor)
			{
				TODManagerListenner* l = *itor;
				if (l)
				{
					l->onTodListChanged(mTodMap);
				}
			}
		}
	}

	void TODManager::deleteTod(const String& name)
	{
		EchoZoneScoped;

		removeTod(name);
	}

	void TODManager::cleanup()
	{
		EchoZoneScoped;

		for (TODMap::iterator it = mTodMap.begin(); it != mTodMap.end(); ++it)
		{
			TODVariableInfo* tm = it->second;

			delete tm;
		}
		mTodMap.clear();

		mColorGradingTextureMap.clear();
		mCloudSeaTextureMap.clear();

		for (TODManagerListenners::iterator it = mListenners.begin(); it != mListenners.end(); ++it)
		{
			TODManagerListenner* l = *it;
			if (l)
			{
				l->onTodListChanged(mTodMap);
			}
		}

		mPreNeedUpdateWeatherParam = false;
		mUseCustomColorChart = false;
		mCustomColorGradingTexturePtr.setNull();
		mDefaultColorGradingTexturePtr.setNull();

		for (int i = 0; i < 5; i++)
		{
			mCurrentDiffuseName[i].clear();
		}

		mCurrentColorChartName = "Default";
		mCurrentColorChartTex = nullptr;

		mCurrentVariableInfo = nullptr;
		mOldVariableInfo = nullptr;

		//将场景中生效的当前场景的colorgrading指针置为空
		setCurrentColorChart("");
		//生效的colorgrading名字还原为默认，在上一步会变为空字符串
		mCurrentColorChartName = "Default";
	}

	void TODManager::setStaticTime(double t)
	{
		EchoZoneScoped;

		t = std::min(std::max(t, 0.0), 1.0);
		mStaticTime = t;
		updateData(0.0, true);
	}

	void TODManager::setRangeStartTime(float t)
	{
		EchoZoneScoped;

		if (mCurrentVariableInfo)
		{
			mCurrentVariableInfo->setRangeStartTime(t);
		}
	}

	float TODManager::getRangeStartTime() const
	{
		EchoZoneScoped;

		if (mCurrentVariableInfo)
		{
			return mCurrentVariableInfo->getRangeStartTime();
		}
		return 0.0f;
	}

	void TODManager::setRangeEndTime(float t)
	{
		EchoZoneScoped;

		if (mCurrentVariableInfo)
		{
			mCurrentVariableInfo->setRangeEndTime(t);
		}
	}

	float TODManager::getRangeEndTime() const
	{
		EchoZoneScoped;

		if (mCurrentVariableInfo)
		{
			return mCurrentVariableInfo->getRangeEndTime();
		}
		return 1.0f;
	}

	bool TODManager::selectTod(const String& name, bool update)
	{
		EchoZoneScoped;

		TODVariableInfo* tod = loadTod(name);
		if (!tod)
			return false;

		mOldVariableInfo = mCurrentVariableInfo;
		mCurrentVariableInfo = tod;

		for (TODManagerListenners::iterator it = mListenners.begin(); it != mListenners.end(); ++it)
		{
			TODManagerListenner* l = *it;
			if (l)
			{
				l->onTodChanged(name, true);
			}
		}
		if (update)
		{
			updateData(0.0, true);
		}
		return true;

	}

	void TODManager::setTranslationTime(float time_)
	{
		EchoZoneScoped;

		mTranslationTime = time_;
		mTranslationTime = std::max(0.01f, mTranslationTime);
	}

	Vector3 TODManager::convertDirectionFromYawPitchAngle(float yaw, float pitch)
	{
		EchoZoneScoped;

		Vector3 dir(0.f);
		dir.x = Math::Cos(Math::DegreesToRadians(pitch));
		dir.y = Math::Sin(Math::DegreesToRadians(pitch));

		Quaternion qua;
		qua.FromAngleAxis(Radian(yaw / 180.f * Math::PI - 2 * Math::PI), Vector3::UNIT_Y);
		dir = qua * dir;
		dir.normalise();

		return dir;
	}

	void TODManager::updateData(double timeElapsed, bool force)
	{
		EchoZoneScoped;

		bool needupdateWeatherParam = getWeatherManager()->isNeedUpdateParam();
		bool preNeedUpdateWeatherParam = mPreNeedUpdateWeatherParam;	// 防止后续逻辑刷新更新状态导致结尾逻辑不会更新组件数据
		if (mIsUseRainState)
		{
			force = (force || needupdateWeatherParam || mPreNeedUpdateWeatherParam);
			mPreNeedUpdateWeatherParam = needupdateWeatherParam;
		}
		bool blerp = mElapseTime < mTranslationTime;
		float factor = 1;

		if (0 == m_dwUpdateTick)
		{
			m_dwUpdateTick = _getCurrentTick();
		}
		else
		{
			unsigned long curTick = _getCurrentTick();
			timeElapsed = (curTick - m_dwUpdateTick) / 1000.0;
			m_dwUpdateTick = curTick;
		}

		mTimeElapsed = timeElapsed;

		if (blerp)
		{
			mElapseTime += (float)timeElapsed;
			factor = mElapseTime / mTranslationTime;
			factor = std::min(1.0f, factor);
			factor = Math::Cos((factor + 1) * Math::PI) * 0.5f + 0.5f;
		}

		m_currentTotalTime += timeElapsed * mTimeSpeed;
		double tempTime = std::fmod(m_currentTotalTime, m_dwDayTime) / m_dwDayTime;

		m_currentRealTotalTime += _getRealElapseTime(tempTime, timeElapsed * mTimeSpeed);
		double sec = std::fmod(m_currentRealTotalTime + (double)mTimePassed + (double)mLocationTimeOffsetSeconds, (double)m_dwDayTime);
		if (sec < 0) sec += m_dwDayTime;
		double time = sec / m_dwDayTime; //时区 西走为后一天（明天）  东走为前一天（昨天）  

		_updateMoonPhase(time); //月相要一直更新

		if (mPlayEnable && mSrcPlayEnable)
		{
			if (mLastUpdateTime < time && time < mLastUpdateTime + mTimeStep && !force && !blerp)
			{
				mCurrentTime = time;
				return;
			}

			if (time > getRangeEndTime())
				time = getRangeStartTime();
			if (time < getRangeStartTime())
				time = getRangeStartTime();
			if (time < 0)
				time = 0;
		}
		else
		{
			if (force || blerp)
			{
				time = lerp(mCurrentTime, mStaticTime, factor);
			}
			else
			{
				return;
			}
		}

		if (!mSceneManager)
		{
			mElapseTime = 0;
			return;
		}

		mCurrentTime = time - (int)time;
		if (mCurrentTime < 1e-10)
			mCurrentTime = 0.0;

		mLastUpdateTime = mCurrentTime;

		if (g_dayFactor < mCurrentTime && mCurrentTime < g_nightFactor)
		{
			mMoonRise = false;
		}
		else
		{
			mMoonRise = true;
		}

		Sun* pSun = mSceneManager->getSun();
		//夜晚将太阳纹理换为月亮的
		if (pSun)
			pSun->setMoonRise(mMoonRise);

		Cloud* pCloud = mSceneManager->getCloud();
		//夜晚将片云纹理换为星星的
		if (pCloud)
			pCloud->setMoonRise(eCloudType_Plane, mMoonRise);

		LensFlare* pLensFlare = mSceneManager->getLensFlare();
		//夜晚将不渲染光晕
		if (pLensFlare)
			pLensFlare->setMoonRise(mMoonRise);

		if (blerp)
		{
			if (mOldVariableInfo)
			{
				SParamValue oldParamValue;
				getParamFromTod(oldParamValue, mOldVariableInfo, false);

				SParamValue  curParamValue;
				getParamFromTod(curParamValue, mCurrentVariableInfo, true);

				float mainLightYaw = lerp(oldParamValue.mMainLightYaw, curParamValue.mMainLightYaw, factor);
				float mainLightPitch = lerp(oldParamValue.mMainLightPitch, curParamValue.mMainLightPitch, factor);

				float auxiliaryYaw = (mainLightYaw + 180.0f);
				float auxiliaryPitch = lerp(oldParamValue.mAuxiliaryLightPitch, curParamValue.mAuxiliaryLightPitch, factor);

				Sky* pSky = mSceneManager->getSky();
				if (pSky)
				{
					pSky->setSkyBottomColour(lerp(oldParamValue.mSkyBottomColor, curParamValue.mSkyBottomColor, factor));
					pSky->setSkyTopColour(lerp(oldParamValue.mSkyTopColor, curParamValue.mSkyTopColor, factor));
					pSky->setSkyColourRatio(lerp(oldParamValue.mSkyColorRatio, curParamValue.mSkyColorRatio, factor));
					pSky->setSkyFogHeight(lerp(oldParamValue.mSkyFogHeight, curParamValue.mSkyFogHeight, factor));
				}
				Stars* pStars = mSceneManager->getStars();
				if (pStars)
				{
					pStars->setStarSize(lerp(oldParamValue.mStarSize, curParamValue.mStarSize, factor));
					pStars->setStarIntensity(lerp(oldParamValue.mStarIntensity, curParamValue.mStarIntensity, factor));
					pStars->setHDRPower(curParamValue.mStarHDRPower);
				}
				VolumetricLight* pVolumetricLight = mSceneManager->getVolumetricLight();
				if (pVolumetricLight)
				{
					pVolumetricLight->setSize(lerp(oldParamValue.mVolumetricLightSize, curParamValue.mVolumetricLightSize, factor));
					pVolumetricLight->setOpacity(lerp(oldParamValue.mVolumetricLightOpacity, curParamValue.mVolumetricLightOpacity, factor));
					pVolumetricLight->setColour(lerp(oldParamValue.mVolumetricLightColour, curParamValue.mVolumetricLightColour, factor));
				}
				if (pLensFlare)
				{
					pLensFlare->setOpacity(lerp(oldParamValue.mLensFlareOpacity, curParamValue.mLensFlareOpacity, factor));
				}

				if (pCloud)
				{
					pCloud->setCloudColour(eCloudType_Sphere, lerp(oldParamValue.mCloudColor, curParamValue.mCloudColor, factor));
					pCloud->setCloudSunBackColour(eCloudType_Sphere, lerp(oldParamValue.mCloudSunBackColor, curParamValue.mCloudSunBackColor, factor));
					pCloud->setCloudUSpeed(eCloudType_Sphere, curParamValue.mCloudSpeed/*lerp(oldParamValue.mCloudSpeed,curParamValue.mCloudSpeed,factor)*/);
					//pCloud->setCloudOpacity(eCloudType_Sphere, curParamValue.mCloudOpactiy /*lerp(0.0f,curParamValue.mCloudOpactiy, factor*factor*factor)*/);
					pCloud->setCloudEmendation(eCloudType_Sphere, lerp(oldParamValue.mCloudEmendation, curParamValue.mCloudEmendation, factor));
					setCurrentCloudDiffuseByType(eCloudType_Sphere, curParamValue.mCloudDiffuse);

					pCloud->setCloudColour(eCloudType_Plane, lerp(oldParamValue.mCloudPlaneColor, curParamValue.mCloudPlaneColor, factor));
					pCloud->setCloudSunBackColour(eCloudType_Plane, lerp(oldParamValue.mCloudPlaneSunBackColor, curParamValue.mCloudPlaneSunBackColor, factor));
					pCloud->setCloudUSpeed(eCloudType_Plane, curParamValue.mCloudPlaneUSpeed/*lerp(oldParamValue.mCloudPlaneUSpeed, curParamValue.mCloudPlaneUSpeed, factor)*/);
					pCloud->setCloudVSpeed(eCloudType_Plane, curParamValue.mCloudPlaneVSpeed/*lerp(oldParamValue.mCloudPlaneVSpeed, curParamValue.mCloudPlaneVSpeed, factor)*/);
					pCloud->setCloudUVTilling(eCloudType_Plane, curParamValue.mCloudPlaneUVTilling/*lerp(oldParamValue.mCloudPlaneUVTilling, curParamValue.mCloudPlaneUVTilling, factor)*/);
					//pCloud->setCloudOpacity(eCloudType_Plane, curParamValue.mCloudPlaneOpactiy /*lerp(0.0f, curParamValue.mCloudPlaneOpactiy, factor*factor*factor)*/);
					pCloud->setCloudEmendation(eCloudType_Plane, lerp(oldParamValue.mCloudPlaneEmendation, curParamValue.mCloudPlaneEmendation, factor));

					pCloud->setCloudColour(eCloudType_Mesh_Up, lerp(oldParamValue.mCloudMeshColor[0], curParamValue.mCloudMeshColor[0], factor));
					//pCloud->setCloudOpacity(eCloudType_Mesh_Up, curParamValue.mCloudMeshOpactiy[0] /*lerp(0.0f, curParamValue.mCloudMeshOpactiy[0], factor*factor*factor)*/);
					pCloud->setCloudEmendation(eCloudType_Mesh_Up, lerp(oldParamValue.mCloudMeshEmendation[0], curParamValue.mCloudMeshEmendation[0], factor));
					pCloud->setCloudSunBackColour(eCloudType_Mesh_Up, lerp(oldParamValue.mCloudMeshSunBackColor[0], curParamValue.mCloudMeshSunBackColor[0], factor));
					setCurrentCloudDiffuseByType(eCloudType_Mesh_Up, curParamValue.mCloudMeshDiffuse[0]);

					pCloud->setCloudColour(eCloudType_Mesh_Mid, lerp(oldParamValue.mCloudMeshColor[1], curParamValue.mCloudMeshColor[1], factor));
					//pCloud->setCloudOpacity(eCloudType_Mesh_Mid, curParamValue.mCloudMeshOpactiy[1] /*lerp(0.0f, curParamValue.mCloudMeshOpactiy[1], factor*factor*factor)*/);
					pCloud->setCloudEmendation(eCloudType_Mesh_Mid, lerp(oldParamValue.mCloudMeshEmendation[1], curParamValue.mCloudMeshEmendation[1], factor));
					pCloud->setCloudSunBackColour(eCloudType_Mesh_Mid, lerp(oldParamValue.mCloudMeshSunBackColor[1], curParamValue.mCloudMeshSunBackColor[1], factor));
					setCurrentCloudDiffuseByType(eCloudType_Mesh_Mid, curParamValue.mCloudMeshDiffuse[1]);

					pCloud->setCloudColour(eCloudType_Mesh_Down, lerp(oldParamValue.mCloudMeshColor[2], curParamValue.mCloudMeshColor[2], factor));
					//pCloud->setCloudOpacity(eCloudType_Mesh_Down, curParamValue.mCloudMeshOpactiy[2] /*lerp(0.0f, curParamValue.mCloudMeshOpactiy[2], factor*factor*factor)*/);
					pCloud->setCloudEmendation(eCloudType_Mesh_Down, lerp(oldParamValue.mCloudMeshEmendation[2], curParamValue.mCloudMeshEmendation[2], factor));
					pCloud->setCloudSunBackColour(eCloudType_Mesh_Down, lerp(oldParamValue.mCloudMeshSunBackColor[2], curParamValue.mCloudMeshSunBackColor[2], factor));
					setCurrentCloudDiffuseByType(eCloudType_Mesh_Down, curParamValue.mCloudMeshDiffuse[2]);
				}

				if (pSun)
				{
					pSun->setEmissionColour(lerp(oldParamValue.mSunEmissionColor, curParamValue.mSunEmissionColor, factor));
					pSun->setSunOuterRadius(curParamValue.mSunOuterRadius/*lerp(oldParamValue.mSunOuterRadius, curParamValue.mSunOuterRadius, factor)*/);
					pSun->setSunInnerRadius(curParamValue.mSunInnerRadius/*lerp(oldParamValue.mSunInnerRadius, curParamValue.mSunInnerRadius, factor)*/);
					pSun->setSunOpacity(curParamValue.mSunOpactiy/*lerp(oldParamValue.mSunOpactiy, curParamValue.mSunOpactiy, factor*factor*factor)*/);
					pSun->setSunDirection(convertDirectionFromYawPitchAngle(curParamValue.mMainLightYaw, curParamValue.mMainLightPitch));
				}

				Vector3 lightDir = convertDirectionFromYawPitchAngle(mainLightYaw, mainLightPitch > mMainLightPitchLimit ? mMainLightPitchLimit : mainLightPitch);

				mSceneManager->setMainLightColor(lerp(oldParamValue.mMainLightColor, curParamValue.mMainLightColor, factor));
				mSceneManager->setMainLightDirection(lightDir);
				mSceneManager->setMainLightShadowDirection(lightDir);

				mSceneManager->setAuxiliaryLightColor(lerp(oldParamValue.mAuxiliaryLightColor, curParamValue.mAuxiliaryLightColor, factor));
				mSceneManager->setAuxiliaryLightDirection(convertDirectionFromYawPitchAngle(auxiliaryYaw, auxiliaryPitch > -10.0 ? -10.0f : auxiliaryPitch));

				mSceneManager->setSkyLightColour(lerp(oldParamValue.mSkyLightColor, curParamValue.mSkyLightColor, factor));
				mSceneManager->setGroundLightColour(lerp(oldParamValue.mGroundLightColor, curParamValue.mGroundLightColor, factor));

				mSceneManager->setPBRMainLightColor(lerp(oldParamValue.mPBRMainLightColor, curParamValue.mPBRMainLightColor, factor));
				mSceneManager->setPBRSkyLightColour(lerp(oldParamValue.mPBRSkyLightColor, curParamValue.mPBRSkyLightColor, factor));
				mSceneManager->setPBRGroundLightColour(lerp(oldParamValue.mPBRGroundLightColor, curParamValue.mPBRGroundLightColor, factor));
				mSceneManager->setPBRAmbientBlendThreshold(lerp(oldParamValue.mPBRAmbientBlendThreshold, curParamValue.mPBRAmbientBlendThreshold, factor));

				mSceneManager->setRoleMainLightColor(lerp(oldParamValue.mRoleMainLightColor, curParamValue.mRoleMainLightColor, factor));
				mSceneManager->setRoleSkyLightColour(lerp(oldParamValue.mRoleSkyLightColor, curParamValue.mRoleSkyLightColor, factor));
				mSceneManager->setRoleGroundLightColour(lerp(oldParamValue.mRoleGroundLightColor, curParamValue.mRoleGroundLightColor, factor));
				mSceneManager->setRoleAmbientBlendThreshold(lerp(oldParamValue.mRoleAmbientBlendThreshold, curParamValue.mRoleAmbientBlendThreshold, factor));

				mSceneManager->setSunColor(lerp(oldParamValue.mSunColor, curParamValue.mSunColor, factor));
				mSceneManager->setShadowWeight(lerp(oldParamValue.mShadowWeight, curParamValue.mShadowWeight, factor));

				mSceneManager->setLightPower(lerp(oldParamValue.mGlobalPointLightPower, curParamValue.mGlobalPointLightPower, factor));
				mSceneManager->setRolePointLightPower(lerp(oldParamValue.mRolePointLightPower, curParamValue.mRolePointLightPower, factor));
				mSceneManager->setRolePointLightColor(lerp(oldParamValue.mRolePointLightColor, curParamValue.mRolePointLightColor, factor));

				mSceneManager->setGlobalSpecularRatio(lerp(oldParamValue.mGlobalSpecularRatio, curParamValue.mGlobalSpecularRatio, factor));
				mSceneManager->setGlobalReflectionRatio(0.1f * lerp(oldParamValue.mGlobalReflectionRatio, curParamValue.mGlobalReflectionRatio, factor));
				//mSceneManager->setGlobalEnvironmentExplosure(lerp(oldParamValue.mGlobalEnvironmentExplosure, curParamValue.mGlobalEnvironmentExplosure, factor));
				mSceneManager->setGlobalCorrectionColor(lerp(oldParamValue.mGlobalCorrectionColor, curParamValue.mGlobalCorrectionColor, factor));
				mSceneManager->setGlobalVegCorrectionColor(lerp(oldParamValue.mGlobalVegCorrectionColor, curParamValue.mGlobalVegCorrectionColor, factor));
				mSceneManager->setGlobalBuildCorrectionColor(lerp(oldParamValue.mGlobalBuildCorrectionColor, curParamValue.mGlobalBuildCorrectionColor, factor));
				mSceneManager->setGlobalTerrainCorrectionColor(lerp(oldParamValue.mGlobalTerrainCorrectionColor, curParamValue.mGlobalTerrainCorrectionColor, factor));

				getEnvironmentLight()->setGlobalEnvironmentExplosure(lerp(oldParamValue.mGlobalEnvironmentExplosure, curParamValue.mGlobalEnvironmentExplosure, factor), EnvironmentLight::DataFromEditor);

				float windPower = curParamValue.mWindPower;// lerp(oldParamValue.mWindPower, curParamValue.mWindPower, factor);
				float windAngle = curParamValue.mWindAngle;// lerp(oldParamValue.mWindAngle, curParamValue.mWindAngle, factor);
				float fRadian = windAngle / 180.f * Math::PI;
				mSceneManager->setWindParams(Vector4(Math::Cos(fRadian), 0, Math::Sin(fRadian), windPower));

				float swayAmp = curParamValue.mWindTreeSwayAmp;		//lerp(oldParamValue.mWindSwayAmp,curParamValue.mWindSwayAmp,factor);
				float swayFreq = curParamValue.mWindTreeSwayFreq;	// lerp(oldParamValue.mWindSwayFreq, curParamValue.mWindSwayFreq, factor);
				float shakeAmp = curParamValue.mWindTreeShakeAmp;	// lerp(oldParamValue.mWindShakeAmp, curParamValue.mWindShakeAmp, factor);
				float shakeFreq = curParamValue.mWindTreeShakeFreq;	// lerp(oldParamValue.mWindShakeFreq, curParamValue.mWindShakeFreq, factor);
				mSceneManager->setGlobalTreeAnimationParams(Vector4(swayAmp, swayFreq, shakeAmp, shakeFreq));

				swayAmp = curParamValue.mWindVegSwayAmp;		//lerp(oldParamValue.mWindSwayAmp,curParamValue.mWindSwayAmp,factor);
				swayFreq = curParamValue.mWindVegSwayFreq;	// lerp(oldParamValue.mWindSwayFreq, curParamValue.mWindSwayFreq, factor);
				shakeAmp = curParamValue.mWindVegShakeAmp;	// lerp(oldParamValue.mWindShakeAmp, curParamValue.mWindShakeAmp, factor);
				shakeFreq = curParamValue.mWindVegShakeFreq;	// lerp(oldParamValue.mWindShakeFreq, curParamValue.mWindShakeFreq, factor);
				mSceneManager->setGlobalVegetationAnimationParams(Vector4(swayAmp, swayFreq, shakeAmp, shakeFreq));

				getWeatherManager()->setCoverColor(curParamValue.mWeatherCoverColor);
				getWeatherManager()->setCoverColorIntensity(curParamValue.mCoverColorIntensity, lerp(oldParamValue.mCoverColorIntensity, curParamValue.mCoverColorIntensity, factor));

				Ocean* pOcean = mSceneManager->getOcean();
				if (pOcean)
				{
					pOcean->setUnderWaterFogColor(lerp(oldParamValue.mOceanUnderWaterFogColor, curParamValue.mOceanUnderWaterFogColor, factor));
					pOcean->setOceanWindSpeed(curParamValue.mOceanWindPower);
					pOcean->setOceanWindDir(windAngle);
				}

				mSceneManager->setWaterShallowColor(lerp(oldParamValue.mOceanShallowColor, curParamValue.mOceanShallowColor, factor));
				mSceneManager->setWaterDeepColor(lerp(oldParamValue.mOceanDeepColor, curParamValue.mOceanDeepColor, factor));
				mSceneManager->setWaterFoamColor(lerp(oldParamValue.mOceanFoamColor, curParamValue.mOceanFoamColor, factor));
				mSceneManager->setWaterReflectColor(lerp(oldParamValue.mOceanReflectColor, curParamValue.mOceanReflectColor, factor));
				mSceneManager->setUnderWaterColor(lerp(oldParamValue.mOceanUnderWaterOceanColor, curParamValue.mOceanUnderWaterOceanColor, factor));
				mSceneManager->setCausticColour(lerp(oldParamValue.mOceanUnderWaterCausticColor, curParamValue.mOceanUnderWaterCausticColor, factor));

				SphericalOcean* planetOcean = mSceneManager->getPlanetManager()->getCurrentOcean();
				if (planetOcean && planetOcean->getIsUseTod())
				{
					planetOcean->setUnderWaterFogColor(lerp(oldParamValue.mOceanUnderWaterFogColor, curParamValue.mOceanUnderWaterFogColor, factor));
					planetOcean->setOceanWindSpeed(curParamValue.mOceanWindPower);
					planetOcean->setOceanWindDir(windAngle);

					planetOcean->setWaterShallowColor(lerp(oldParamValue.mOceanShallowColor, curParamValue.mOceanShallowColor, factor));
					planetOcean->setWaterDeepColor(lerp(oldParamValue.mOceanDeepColor, curParamValue.mOceanDeepColor, factor));
					planetOcean->setWaterFoamColor(lerp(oldParamValue.mOceanFoamColor, curParamValue.mOceanFoamColor, factor));
					planetOcean->setWaterReflectColor(lerp(oldParamValue.mOceanReflectColor, curParamValue.mOceanReflectColor, factor));
				}

				const int fogSec = 4;
				ColorValue fogColor[fogSec * 2];
				float fogStart[fogSec * 2], fogEnd[fogSec * 2];
				float fogDensity[fogSec * 2], fogTopHeight[fogSec * 2];

				for (int i = 0; i < fogSec; i++)
				{
					fogColor[i] = lerp(oldParamValue.mFogNearColor[i], curParamValue.mFogNearColor[i], factor);
					fogStart[i] = lerp(oldParamValue.mFogNearStart[i], curParamValue.mFogNearStart[i], factor);
					fogEnd[i] = lerp(oldParamValue.mFogNearEnd[i], curParamValue.mFogNearEnd[i], factor);
					fogDensity[i] = lerp(oldParamValue.mFogNearDensity[i], curParamValue.mFogNearDensity[i], factor);
					fogTopHeight[i] = lerp(oldParamValue.mFogNearTopHeight[i], curParamValue.mFogNearTopHeight[i], factor);
					fogColor[i + fogSec] = lerp(oldParamValue.mFogFarColor[i], curParamValue.mFogFarColor[i], factor);
					fogStart[i + fogSec] = lerp(oldParamValue.mFogFarStart[i], curParamValue.mFogFarStart[i], factor);
					fogEnd[i + fogSec] = lerp(oldParamValue.mFogFarEnd[i], curParamValue.mFogFarEnd[i], factor);
					fogDensity[i + fogSec] = lerp(oldParamValue.mFogFarDensity[i], curParamValue.mFogFarDensity[i], factor);
					fogTopHeight[i + fogSec] = lerp(oldParamValue.mFogFarTopHeight[i], curParamValue.mFogFarTopHeight[i], factor);
				}
				mSceneManager->setFog(mSceneManager->getFogMode(), fogColor, fogDensity, fogStart, fogEnd, fogTopHeight);

				//只有是激活场景才修改后期效果
				if (Root::instance()->getActiveSceneManager() == mSceneManager)
				{
					setCurrentColorChart(curParamValue.mColorGradingTextureName);

					float bloomThreshold = lerp(oldParamValue.mBloomThreshold, curParamValue.mBloomThreshold, factor);
					float bloomStrength = lerp(oldParamValue.mBloomBlurStrength, curParamValue.mBloomBlurStrength, factor);
					float bloomRadius = lerp(oldParamValue.mBloomBlurRadius, curParamValue.mBloomBlurRadius, factor);
					_setBloomParam(bloomThreshold, bloomStrength, bloomRadius);

					_setHDRDefaultParam(curParamValue);
					_setHDRParam(curParamValue);
				}

				AtmosphereProfile atmosProfile;
				atmosProfile.lowerLayerColor = lerp(oldParamValue.mAtmosphereLowerLayerColor, curParamValue.mAtmosphereLowerLayerColor, factor);
				atmosProfile.upperLayerColor = lerp(oldParamValue.mAtmosphereUpperLayerColor, curParamValue.mAtmosphereUpperLayerColor, factor);
				atmosProfile.blendParameter.x = lerp(oldParamValue.mAtmosphereHeight, curParamValue.mAtmosphereHeight, factor);
				atmosProfile.blendParameter.y = lerp(oldParamValue.mAtmosphereBlendRange, curParamValue.mAtmosphereBlendRange, factor);
				atmosProfile.blendParameter.z = lerp(oldParamValue.mAtmosphereMaxAlpha, curParamValue.mAtmosphereMaxAlpha, factor);
				atmosProfile.absorptionProfile[1].expTerm = lerp(oldParamValue.mAtmosphereTransmittanceExpTerm, curParamValue.mAtmosphereTransmittanceExpTerm, factor);
				atmosProfile.absorptionProfile[1].expScale = lerp(oldParamValue.mAtmosphereTransmittanceExpScale, curParamValue.mAtmosphereTransmittanceExpScale, factor);
				//SphericalTerrainAtmosphereManager::instance()->setProfile(atmosProfile);

				std::array<float, 3> percent;
				percent[0] = (lerp(oldParamValue.mViewLightPercent[0], curParamValue.mViewLightPercent[0], factor));
				percent[1] = (lerp(oldParamValue.mViewLightPercent[1], curParamValue.mViewLightPercent[1], factor));
				percent[2] = (lerp(oldParamValue.mViewLightPercent[2], curParamValue.mViewLightPercent[2], factor));

				mSceneManager->setViewLightPercent(percent);
			}
			else
			{
				if (mCurrentVariableInfo)
				{
					mCurrentVariableInfo->setTime(mCurrentTime);
				}
				else
				{
					mCurrentColorChartTex = _getDefaultColorChart();
				}
			}
		}
		else
		{
			if (mCurrentVariableInfo)
			{
				mCurrentVariableInfo->setTime(mCurrentTime);
			}
			else
			{
				mCurrentColorChartTex = _getDefaultColorChart();
			}
		}



		getWeatherManager()->updateWeatherParam((float)timeElapsed);

		for (TODManagerListenners::iterator it = mListenners.begin(); it != mListenners.end(); ++it)
		{
			TODManagerListenner* l = *it;
			if (l)
			{
				l->onTimeUpdate(mCurrentTime);
			}
		}

		if (Root::instance()->getRuntimeMode() == Root::eRM_Editor && mIsUseRainState && (needupdateWeatherParam || preNeedUpdateWeatherParam))
			ProtocolComponentSystem::instance()->callProtocolFunc(ProtocolFuncID::PFID_ResetTodComAttribute, &mName, 0);
	}

	//void TODManager::_setColorGrading(Texture * colorChartTex)
	//{
	//	RenderSystem* pRenderSystem = Root::instance()->getRenderSystem();
	//	if (colorChartTex && pRenderSystem)
	//	{
	//		RenderStrategy*	pStrategy = pRenderSystem->getRenderStrategy();
	//		if (pStrategy)
	//		{
	//			PostProcessChain* pProcessChain = pStrategy->getPostProcessChain();
	//			if (pProcessChain)
	//			{
	//				ColorGrading* pColorGrading = nullptr;
	//				pColorGrading = static_cast<ColorGrading*>(pProcessChain->getPostProcess(PostProcess::Type::ColorGrading));
	//				if (pColorGrading)
	//				{
	//					pColorGrading->setColorChart(colorChartTex);
	//				}
	//			}
	//		}
	//	}
	//}

	void TODManager::_setBloomParam(float threshold, float strength, float radius)
	{
		EchoZoneScoped;

		RenderSystem* pRenderSystem = Root::instance()->getRenderSystem();
		if (!pRenderSystem)
			return;

		RenderStrategy* pStrategy = pRenderSystem->getCurRenderStrategy();
		if (!pStrategy)
			return;

		PostProcessChain* pProcessChain = pStrategy->getPostProcessChain();
		if (!pProcessChain)
			return;

		Bloom* pBloom = nullptr;
		pBloom = static_cast<Bloom*>(pProcessChain->getPostProcess(PostProcess::Type::Bloom));
		if (!pBloom)
			return;

		pBloom->setBloomLuminanceThreshold(threshold);
		pBloom->setBloomGaussianBlurStrength(strength);
		pBloom->setBloomGaussianBlurRadius(radius);
	}

	void TODManager::_setHDRParam(const SParamValue& param)
	{
		EchoZoneScoped;

		RenderSystem* pRenderSystem = Root::instance()->getRenderSystem();
		if (!pRenderSystem)
			return;

		RenderStrategy* pStrategy = pRenderSystem->getCurRenderStrategy();
		if (!pStrategy)
			return;

		PostProcessChain* pProcessChain = pStrategy->getPostProcessChain();
		if (!pProcessChain)
			return;

		HDR* pHDR = static_cast<HDR*>(pProcessChain->getPostProcess(PostProcess::Type::HDR));
		if (nullptr != pHDR)
		{
			pHDR->setEyeAdaptationParam0x(param.mHDREyeParam[0]);
			pHDR->setEyeAdaptationParam0y(param.mHDREyeParam[1]);
			pHDR->setEyeAdaptationParam0z(param.mHDREyeParam[2]);
			pHDR->setEyeAdaptationParam0w(param.mHDREyeParam[3]);

			pHDR->setBrightParam0x(param.mHDRBrightParam[0]);
			pHDR->setBrightParam0y(param.mHDRBrightParam[1]);
			pHDR->setBrightParam0z(param.mHDRBrightParam[2]);
			pHDR->setBrightParam0w(param.mHDRBrightParam[3]);


			pHDR->setToneMappingParam0x(param.mHDRToneMappingParam[0]);
			pHDR->setToneMappingParam0y(param.mHDRToneMappingParam[1]);
			pHDR->setToneMappingParam0z(param.mHDRToneMappingParam[2]);

			pHDR->setToneMappingCurve(
				param.mHDRToneMappingCurve[0],
				param.mHDRToneMappingCurve[1],
				param.mHDRToneMappingCurve[2],
				param.mHDRToneMappingCurve[3],
				param.mHDRToneMappingCurve[4]);

			pHDR->setToneMappingColorBalance(
				param.mHDRToneMappingColor[0],
				param.mHDRToneMappingColor[1],
				param.mHDRToneMappingColor[2]);
		}
	}

	void TODManager::_setHDRDefaultParam(SParamValue& param)
	{
		param.mHDREyeParam[0] = 0.15f;//   0.1f;
		param.mHDREyeParam[1] = 0.7f;//  0.1f;
		param.mHDREyeParam[2] = 0.5f;// 1.5f;
		param.mHDREyeParam[3] = 0.05f;

		param.mHDRBrightParam[1] = 0.5f;
		param.mHDRBrightParam[2] = 1.0f;
		param.mHDRBrightParam[3] = 3.3f;

		param.mHDRToneMappingParam[1] = 0.5f;
		param.mHDRToneMappingParam[2] = 1.0f;

		param.mHDRToneMappingCurve[0] = 3.3f;
		param.mHDRToneMappingCurve[1] = 0.08f;
		param.mHDRToneMappingCurve[2] = 2.43f;
		param.mHDRToneMappingCurve[3] = 0.59f;

		param.mHDRToneMappingColor[0] = 1.0f;
		param.mHDRToneMappingColor[1] = 1.0f;
		param.mHDRToneMappingColor[2] = 1.0f;
	}

	void TODManager::_updateMoonPhase(double time)
	{
		EchoZoneScoped;

		if (!mSceneManager)
			return;

		double curTime = time - (int)time;
		if (curTime < 1e-10)
			curTime = 0.0;

		bool bMoonRise = false;
		if (g_dayFactor < curTime && curTime < g_nightFactor)
		{
			bMoonRise = false;
		}
		else
		{
			bMoonRise = true;
		}

		Sun* pSun = mSceneManager->getSun();
		if (pSun)
			pSun->updateMoonPhase(bMoonRise);
	}

	double TODManager::getTimeElapsed()
	{
		return mTimeElapsed;
	}

	void TODManager::setLocationTimeOffsetSeconds(int seconds)
	{
		mLocationTimeOffsetSeconds = seconds;
	}

	bool TODManager::judgeIsFirstUpdate()
	{
		if (mCurrentVariableInfo == nullptr)
			return true;
		return false;
	}

	void TODManager::getActiveColorChartName(std::string& name_)
	{
		if (mUseCustomColorChart)
		{
			if (!mCustomColorGradingTexturePtr.isNull())
				name_ = mCustomColorGradingTexturePtr->getName().c_str();
		}
		else
		{
			name_ = mCurrentColorChartName.c_str();
		}
	}

	void TODManager::setErrorId(int id_, const std::string& areaName_)
	{
		m_ErrorIdMap.emplace(std::make_pair(areaName_, id_));
	}

	void TODManager::_updateTimePeriodSpeed()
	{
		EchoZoneScoped;

		for (size_t i = 0, count = mTimeSpeedSlots.size(); i < count; i++)
		{
			TODTimeSpeedSlot& slot = mTimeSpeedSlots[i];
			slot.mSpeed = (slot.mEnd - slot.mStart) / slot.mPercent;
			if (i == 0)
			{
				slot.mRealStart = slot.mStart;
				slot.mRealEnd = slot.mPercent;
			}
			else
			{
				slot.mRealStart = mTimeSpeedSlots[i - 1].mRealEnd;
				slot.mRealEnd = slot.mRealStart + slot.mPercent;
			}
		}
	}

	double TODManager::_getRealElapseTime(double curTime, double elapsedTime)
	{
		EchoZoneScoped;

		double realElapsedtime = 0.0;
		for (size_t i = 0, count = mTimeSpeedSlots.size(); i < count; i++)
		{
			const TODTimeSpeedSlot& slot = mTimeSpeedSlots[i];
			if (slot.mRealStart < curTime && curTime < slot.mRealEnd)
			{
				realElapsedtime = elapsedTime * slot.mSpeed;
				break;
			}
		}
		return realElapsedtime;
	}

	int64 TODManager::_getReferenceTime()
	{
		EchoZoneScoped;

		struct tm t;
		t.tm_year = 2018 - 1900;
		t.tm_mon = 0;
		t.tm_mday = 1;
		t.tm_hour = 0;
		t.tm_min = 0;
		t.tm_sec = 0;
		t.tm_isdst = 0;
		return mktime(&t);
	}

	unsigned long TODManager::_getCurrentTick()
	{
		EchoZoneScoped;

		unsigned long curTick = 0;
#ifdef _WIN32
		curTick = GetTickCount();
#else
		struct timeval tv;
		gettimeofday(&tv, 0);
		curTick = tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif

		return curTick;
	}

	void TODManager::prepareTexture(TODVariableInfo* pVariableInfo)
	{
		EchoZoneScoped;

		_onPrepareTexture(pVariableInfo, TODVariableInfo::PARAM_POST_PROCESS_COLOR_GRADING, mColorGradingTextureMap);

		_onPrepareTexture(pVariableInfo, TODVariableInfo::PARAM_CLOUD_TEXTURE, mCloudSeaTextureMap);
		_onPrepareTexture(pVariableInfo, TODVariableInfo::PARAM_CLOUD_MESH_UP_TEXTURE, mCloudSeaTextureMap);
		_onPrepareTexture(pVariableInfo, TODVariableInfo::PARAM_CLOUD_MESH_MID_TEXTURE, mCloudSeaTextureMap);
		_onPrepareTexture(pVariableInfo, TODVariableInfo::PARAM_CLOUD_MESH_DOWN_TEXTURE, mCloudSeaTextureMap);
	}

	void TODManager::loadTexture(TODVariableInfo* pVariableInfo)
	{
		EchoZoneScoped;

		_onLoadTexture(pVariableInfo, TODVariableInfo::PARAM_POST_PROCESS_COLOR_GRADING, mColorGradingTextureMap);

		_onLoadTexture(pVariableInfo, TODVariableInfo::PARAM_CLOUD_TEXTURE, mCloudSeaTextureMap);
		_onLoadTexture(pVariableInfo, TODVariableInfo::PARAM_CLOUD_MESH_UP_TEXTURE, mCloudSeaTextureMap);
		_onLoadTexture(pVariableInfo, TODVariableInfo::PARAM_CLOUD_MESH_MID_TEXTURE, mCloudSeaTextureMap);
		_onLoadTexture(pVariableInfo, TODVariableInfo::PARAM_CLOUD_MESH_DOWN_TEXTURE, mCloudSeaTextureMap);
	}

	void TODManager::_onPrepareTexture(TODVariableInfo* pVariableInfo, TODVariableInfo::ETimeOfDayParamID paramId, std::map<String, TexturePtr>& texMap)
	{
		EchoZoneScoped;

		TODVariableInfo::SVariableInfo svarInfo;
		pVariableInfo->getVariableInfo(paramId, svarInfo);
		if (svarInfo.pParamTrace)
		{
			for (int i = 0; i < svarInfo.pParamTrace->getKeyCount(); i++)
			{
				Echo::String sValue;
				svarInfo.pParamTrace->getKeyValueString(i, sValue);
				if (sValue.empty())
					continue;

				std::map<String, TexturePtr>::iterator it = texMap.find(sValue);
				if (it == texMap.end())
				{
					TexturePtr textPtr = TextureManager::instance()->prepareTexture(Name(sValue), TEX_TYPE_2D);
					if (!textPtr.isNull())
						texMap.insert(make_pair(sValue, textPtr));
				}
			}
		}
	}

	void TODManager::_onLoadTexture(TODVariableInfo* pVariableInfo, TODVariableInfo::ETimeOfDayParamID paramId, std::map<String, TexturePtr>& texMap)
	{
		EchoZoneScoped;

		TODVariableInfo::SVariableInfo svarInfo;
		pVariableInfo->getVariableInfo(paramId, svarInfo);
		pVariableInfo->setTextureLoaded(true);
		if (svarInfo.pParamTrace)
		{
			for (int i = 0; i < svarInfo.pParamTrace->getKeyCount(); i++)
			{
				Echo::String sValue;
				svarInfo.pParamTrace->getKeyValueString(i, sValue);
				if (sValue.empty())
					continue;

				std::map<String, TexturePtr>::iterator it = texMap.find(sValue);
				if (it == texMap.end())
				{
					TexturePtr textPtr = TextureManager::instance()->loadTexture(Name(sValue), TEX_TYPE_2D);
					if (!textPtr.isNull())
						texMap.insert(make_pair(sValue, textPtr));
				}
				else
				{
					it->second->load();
				}
			}
		}
	}

	void TODManager::getParamFromTod(SParamValue& paramInfo, TODVariableInfo* pInfo, bool bLerp)
	{
		EchoZoneScoped;

		if (bLerp)
		{
			pInfo->setTime(mCurrentTime, false);
			//pInfo->update(true,false);
		}

		//-----------------------------------------------------------------------------
		bool bEnableHDR = false;
		RenderStrategy* pStrategy = Root::instance()->getRenderSystem()->getCurRenderStrategy();
		if (pStrategy && pStrategy->getPostProcessChain())
		{
			bEnableHDR = pStrategy->getPostProcessChain()->isEnable(PostProcess::HDR);
		}

		float fMultiplier = 1.0f;
		TODVariableInfo::SVariableInfo varInfo;
		pInfo->getVariableInfo(TODVariableInfo::PARAM_SUN_COLOR_POWER, varInfo);
		fMultiplier = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_SUN_COLOR_HDR_POWER, varInfo);
		float sunHDRPower = varInfo.fValue[0];
		if (bEnableHDR)
		{
			fMultiplier *= sunHDRPower;
		}
		pInfo->getVariableInfo(TODVariableInfo::PARAM_SUN_COLOR, varInfo);
		paramInfo.mSunColor = (fMultiplier * ColorValue(varInfo.fValue[0],
			varInfo.fValue[1], varInfo.fValue[2]));
		pInfo->getVariableInfo(TODVariableInfo::PARAM_SUN_EMISSION_COLOR, varInfo);
		paramInfo.mSunEmissionColor = ColorValue(varInfo.fValue[0], varInfo.fValue[1], varInfo.fValue[2]);
		pInfo->getVariableInfo(TODVariableInfo::PARAM_SUN_RADIUS, varInfo);
		paramInfo.mSunOuterRadius = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_SUN_RADIUS2, varInfo);
		paramInfo.mSunInnerRadius = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_SUN_OPACITY, varInfo);
		paramInfo.mSunOpactiy = varInfo.fValue[0];

		pInfo->getVariableInfo(TODVariableInfo::PARAM_SKY_TOP_COLOR, varInfo);
		paramInfo.mSkyTopColor = ColorValue(varInfo.fValue[0], varInfo.fValue[1], varInfo.fValue[2]);
		pInfo->getVariableInfo(TODVariableInfo::PARAM_SKY_BOTTOM_COLOR, varInfo);
		paramInfo.mSkyBottomColor = ColorValue(varInfo.fValue[0], varInfo.fValue[1], varInfo.fValue[2]);
		pInfo->getVariableInfo(TODVariableInfo::PARAM_SKY_COLOR_RATIO, varInfo);
		paramInfo.mSkyColorRatio = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_SKY_FOG_HEIGHT, varInfo);
		paramInfo.mSkyFogHeight = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_SKY_BRIGHT_RANGE, varInfo);
		paramInfo.mSkyBottomColor.a = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_SKY_BRIGHT_INTENSITY, varInfo);
		paramInfo.mSkyTopColor.a = varInfo.fValue[0];

		pInfo->getVariableInfo(TODVariableInfo::PARAM_STAR_SIZE, varInfo);
		paramInfo.mStarSize = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_STAR_INTENSITY, varInfo);
		paramInfo.mStarIntensity = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_STAR_COLOR_HDR_POWER, varInfo);
		paramInfo.mStarHDRPower = bEnableHDR ? varInfo.fValue[0] : 1.0f;

		pInfo->getVariableInfo(TODVariableInfo::PARAM_VOLUMETRIC_LIGHT_SIZE, varInfo);
		paramInfo.mVolumetricLightSize = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_VOLUMETRIC_LIGHT_OPACITY, varInfo);
		paramInfo.mVolumetricLightOpacity = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_VOLUMETRIC_LIGHT_HDR_POWER, varInfo);
		fMultiplier = 1.0f;
		float volumetricLightHDRPower = varInfo.fValue[0];
		if (bEnableHDR)
		{
			fMultiplier *= volumetricLightHDRPower;
		}
		pInfo->getVariableInfo(TODVariableInfo::PARAM_VOLUMETRIC_LIGHT_COLOR, varInfo);
		paramInfo.mVolumetricLightColour = (fMultiplier * ColorValue(varInfo.fValue[0], varInfo.fValue[1], varInfo.fValue[2]));

		pInfo->getVariableInfo(TODVariableInfo::PARAM_LENS_FLARE_OPACITY, varInfo);
		paramInfo.mLensFlareOpacity = varInfo.fValue[0];

		//Sphere
		pInfo->getVariableInfo(TODVariableInfo::PARAM_CLOUD_SPEED, varInfo);
		paramInfo.mCloudSpeed = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_CLOUD_OPACITY, varInfo);
		paramInfo.mCloudOpactiy = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_CLOUD_EMENDATION, varInfo);
		paramInfo.mCloudEmendation = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_CLOUD_COLOR_POWER, varInfo);
		fMultiplier = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_CLOUD_HDR_POWER, varInfo);
		float cloudSphereHDRPower = varInfo.fValue[0];
		if (bEnableHDR)
		{
			fMultiplier *= cloudSphereHDRPower;
		}
		pInfo->getVariableInfo(TODVariableInfo::PARAM_CLOUD_COLOR, varInfo);
		paramInfo.mCloudColor = (fMultiplier * ColorValue(varInfo.fValue[0],
			varInfo.fValue[1], varInfo.fValue[2]));
		pInfo->getVariableInfo(TODVariableInfo::PARAM_CLOUD_SUNBACK_COLOR_POWER, varInfo);
		fMultiplier = varInfo.fValue[0];
		if (bEnableHDR)
		{
			fMultiplier *= cloudSphereHDRPower;
		}
		pInfo->getVariableInfo(TODVariableInfo::PARAM_CLOUD_SUNBACK_COLOR, varInfo);
		paramInfo.mCloudSunBackColor = (fMultiplier * ColorValue(varInfo.fValue[0],
			varInfo.fValue[1], varInfo.fValue[2]));
		pInfo->getVariableInfo(TODVariableInfo::PARAM_CLOUD_TEXTURE, varInfo);
		paramInfo.mCloudDiffuse = varInfo.sValue;

		//plane
		pInfo->getVariableInfo(TODVariableInfo::PARAM_CLOUD_PLANE_U_ROLLSPEED, varInfo);
		paramInfo.mCloudPlaneUSpeed = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_CLOUD_PLANE_V_ROLLSPEED, varInfo);
		paramInfo.mCloudPlaneVSpeed = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_CLOUD_PLANE_UV_TILLING, varInfo);
		paramInfo.mCloudPlaneUVTilling = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_CLOUD_PLANE_OPACITY, varInfo);
		paramInfo.mCloudPlaneOpactiy = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_CLOUD_PLANE_EMENDATION, varInfo);
		paramInfo.mCloudPlaneEmendation = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_CLOUD_PLANE_COLOR_POWER, varInfo);
		fMultiplier = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_CLOUD_PLANE_HDR_POWER, varInfo);
		float cloudPlaneHDRPower = varInfo.fValue[0];
		if (bEnableHDR)
		{
			fMultiplier *= cloudPlaneHDRPower;
		}
		pInfo->getVariableInfo(TODVariableInfo::PARAM_CLOUD_PLANE_COLOR, varInfo);
		paramInfo.mCloudPlaneColor = (fMultiplier * ColorValue(varInfo.fValue[0],
			varInfo.fValue[1], varInfo.fValue[2]));
		pInfo->getVariableInfo(TODVariableInfo::PARAM_CLOUD_PLANE_SUNBACK_COLOR_POWER, varInfo);
		fMultiplier = varInfo.fValue[0];
		if (bEnableHDR)
		{
			fMultiplier *= cloudPlaneHDRPower;
		}
		pInfo->getVariableInfo(TODVariableInfo::PARAM_CLOUD_PLANE_SUNBACK_COLOR, varInfo);
		paramInfo.mCloudPlaneSunBackColor = (fMultiplier * ColorValue(varInfo.fValue[0],
			varInfo.fValue[1], varInfo.fValue[2]));

		//mesh up
		pInfo->getVariableInfo(TODVariableInfo::PARAM_CLOUD_MESH_UP_OPACITY, varInfo);
		paramInfo.mCloudMeshOpactiy[0] = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_CLOUD_MESH_UP_EMENDATION, varInfo);
		paramInfo.mCloudMeshEmendation[0] = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_CLOUD_MESH_UP_COLOR_POWER, varInfo);
		fMultiplier = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_CLOUD_MESH_UP_HDR_POWER, varInfo);
		float cloudMeshUpHDRPower = varInfo.fValue[0];
		if (bEnableHDR)
		{
			fMultiplier *= cloudMeshUpHDRPower;
		}
		pInfo->getVariableInfo(TODVariableInfo::PARAM_CLOUD_MESH_UP_COLOR, varInfo);
		paramInfo.mCloudMeshColor[0] = (fMultiplier * ColorValue(varInfo.fValue[0],
			varInfo.fValue[1], varInfo.fValue[2]));
		pInfo->getVariableInfo(TODVariableInfo::PARAM_CLOUD_MESH_UP_SUNBACK_COLOR_POWER, varInfo);
		fMultiplier = varInfo.fValue[0];
		if (bEnableHDR)
		{
			fMultiplier *= cloudMeshUpHDRPower;
		}
		pInfo->getVariableInfo(TODVariableInfo::PARAM_CLOUD_MESH_UP_SUNBACK_COLOR, varInfo);
		paramInfo.mCloudMeshSunBackColor[0] = (fMultiplier * ColorValue(varInfo.fValue[0],
			varInfo.fValue[1], varInfo.fValue[2]));
		pInfo->getVariableInfo(TODVariableInfo::PARAM_CLOUD_MESH_UP_TEXTURE, varInfo);
		paramInfo.mCloudMeshDiffuse[0] = varInfo.sValue;

		//mesh mid
		pInfo->getVariableInfo(TODVariableInfo::PARAM_CLOUD_MESH_MID_OPACITY, varInfo);
		paramInfo.mCloudMeshOpactiy[1] = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_CLOUD_MESH_MID_EMENDATION, varInfo);
		paramInfo.mCloudMeshEmendation[1] = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_CLOUD_MESH_MID_COLOR_POWER, varInfo);
		fMultiplier = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_CLOUD_MESH_MID_HDR_POWER, varInfo);
		float cloudMeshMidHDRPower = varInfo.fValue[0];
		if (bEnableHDR)
		{
			fMultiplier *= cloudMeshMidHDRPower;
		}
		pInfo->getVariableInfo(TODVariableInfo::PARAM_CLOUD_MESH_MID_COLOR, varInfo);
		paramInfo.mCloudMeshColor[1] = (fMultiplier * ColorValue(varInfo.fValue[0],
			varInfo.fValue[1], varInfo.fValue[2]));
		pInfo->getVariableInfo(TODVariableInfo::PARAM_CLOUD_MESH_MID_SUNBACK_COLOR_POWER, varInfo);
		fMultiplier = varInfo.fValue[0];
		if (bEnableHDR)
		{
			fMultiplier *= cloudMeshMidHDRPower;
		}
		pInfo->getVariableInfo(TODVariableInfo::PARAM_CLOUD_MESH_MID_SUNBACK_COLOR, varInfo);
		paramInfo.mCloudMeshSunBackColor[1] = (fMultiplier * ColorValue(varInfo.fValue[0],
			varInfo.fValue[1], varInfo.fValue[2]));
		pInfo->getVariableInfo(TODVariableInfo::PARAM_CLOUD_MESH_MID_TEXTURE, varInfo);
		paramInfo.mCloudMeshDiffuse[1] = varInfo.sValue;

		//mesh down
		pInfo->getVariableInfo(TODVariableInfo::PARAM_CLOUD_MESH_DOWN_OPACITY, varInfo);
		paramInfo.mCloudMeshOpactiy[2] = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_CLOUD_MESH_DOWN_EMENDATION, varInfo);
		paramInfo.mCloudMeshEmendation[2] = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_CLOUD_MESH_DOWN_COLOR_POWER, varInfo);
		fMultiplier = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_CLOUD_MESH_DOWN_HDR_POWER, varInfo);
		float cloudMeshDownHDRPower = varInfo.fValue[0];
		if (bEnableHDR)
		{
			fMultiplier *= cloudMeshDownHDRPower;
		}
		pInfo->getVariableInfo(TODVariableInfo::PARAM_CLOUD_MESH_DOWN_COLOR, varInfo);
		paramInfo.mCloudMeshColor[2] = (fMultiplier * ColorValue(varInfo.fValue[0],
			varInfo.fValue[1], varInfo.fValue[2]));
		pInfo->getVariableInfo(TODVariableInfo::PARAM_CLOUD_MESH_DOWN_SUNBACK_COLOR_POWER, varInfo);
		fMultiplier = varInfo.fValue[0];
		if (bEnableHDR)
		{
			fMultiplier *= cloudMeshDownHDRPower;
		}
		pInfo->getVariableInfo(TODVariableInfo::PARAM_CLOUD_MESH_DOWN_SUNBACK_COLOR, varInfo);
		paramInfo.mCloudMeshSunBackColor[2] = (fMultiplier * ColorValue(varInfo.fValue[0],
			varInfo.fValue[1], varInfo.fValue[2]));
		pInfo->getVariableInfo(TODVariableInfo::PARAM_CLOUD_MESH_DOWN_TEXTURE, varInfo);
		paramInfo.mCloudMeshDiffuse[2] = varInfo.sValue;

		pInfo->getVariableInfo(TODVariableInfo::PARAM_MAIN_LIGHT_YAW, varInfo);
		paramInfo.mMainLightYaw = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_MAIN_LIGHT_PITCH, varInfo);
		paramInfo.mMainLightPitch = varInfo.fValue[0];

		ColorValue realColor;
		//-----------------------------------------------------------------------------
		{
			pInfo->getVariableInfo(TODVariableInfo::PARAM_MAIN_LIGHT_COLOR_POWER, varInfo);
			fMultiplier = varInfo.fValue[0];

			pInfo->getVariableInfo(TODVariableInfo::PARAM_MAIN_LIGHT_COLOR, varInfo);
			realColor = ColorValue(varInfo.fValue[0], varInfo.fValue[1], varInfo.fValue[2]);

			if (bEnableHDR)
			{
				pInfo->getVariableInfo(TODVariableInfo::PARAM_MAIN_LIGHT_HDR_POWER, varInfo);
				fMultiplier *= varInfo.fValue[0];
			}

			paramInfo.mMainLightColor = (ColorValue::gammaToLinear(realColor) * fMultiplier);
		}

		//-----------------------------------------------------------------------------
		{
			pInfo->getVariableInfo(TODVariableInfo::PARAM_PBR_MAIN_LIGHT_COLOR_POWER, varInfo);
			fMultiplier = varInfo.fValue[0];

			pInfo->getVariableInfo(TODVariableInfo::PARAM_PBR_MAIN_LIGHT_COLOR, varInfo);
			realColor = ColorValue(varInfo.fValue[0], varInfo.fValue[1], varInfo.fValue[2]);

			if (bEnableHDR)
			{
				pInfo->getVariableInfo(TODVariableInfo::PARAM_PBR_MAIN_LIGHT_HDR_POWER, varInfo);
				fMultiplier *= varInfo.fValue[0];
			}

			paramInfo.mPBRMainLightColor = (ColorValue::gammaToLinear(realColor) * fMultiplier);
		}

		pInfo->getVariableInfo(TODVariableInfo::PARAM_PBR_AMBIENT_BLEND_THRESHOLD, varInfo);
		paramInfo.mPBRAmbientBlendThreshold = varInfo.fValue[0];

		//-----------------------------------------------------------------------------
		{
			pInfo->getVariableInfo(TODVariableInfo::PARAM_ROLE_MAIN_LIGHT_COLOR_POWER, varInfo);
			fMultiplier = varInfo.fValue[0];

			if (bEnableHDR)
			{
				pInfo->getVariableInfo(TODVariableInfo::PARAM_ROLE_MAIN_LIGHT_HDR_POWER, varInfo);
				fMultiplier *= varInfo.fValue[0];
			}

			pInfo->getVariableInfo(TODVariableInfo::PARAM_ROLE_MAIN_LIGHT_COLOR, varInfo);
			realColor = ColorValue(varInfo.fValue[0], varInfo.fValue[1], varInfo.fValue[2]);
			paramInfo.mRoleMainLightColor = (ColorValue::gammaToLinear(realColor) * fMultiplier);
		}

		pInfo->getVariableInfo(TODVariableInfo::PARAM_ROLE_AMBIENT_BLEND_THRESHOLD, varInfo);
		paramInfo.mRoleAmbientBlendThreshold = varInfo.fValue[0];

		//-----------------------------------------------------------------------------
		pInfo->getVariableInfo(TODVariableInfo::PARAM_AUXILIARY_LIGHT_PITCH, varInfo);
		paramInfo.mAuxiliaryLightPitch = varInfo.fValue[0];

		//-----------------------------------------------------------------------------
		{
			pInfo->getVariableInfo(TODVariableInfo::PARAM_AUXILIARY_LIGHT_COLOR_POWER, varInfo);
			fMultiplier = varInfo.fValue[0];

			if (bEnableHDR)
			{
				pInfo->getVariableInfo(TODVariableInfo::PARAM_AUXILIARY_LIGHT_HDR_POWER, varInfo);
				fMultiplier *= varInfo.fValue[0];
			}

			pInfo->getVariableInfo(TODVariableInfo::PARAM_AUXILIARY_LIGHT_COLOR, varInfo);
			realColor = ColorValue(varInfo.fValue[0], varInfo.fValue[1], varInfo.fValue[2]);
			paramInfo.mAuxiliaryLightColor = (ColorValue::gammaToLinear(realColor) * fMultiplier);
		}

		//-----------------------------------------------------------------------------
		{
			pInfo->getVariableInfo(TODVariableInfo::PARAM_PBR_AMBIENT_BLEND_THRESHOLD, varInfo);
			paramInfo.mPBRAmbientBlendThreshold = varInfo.fValue[0];

			pInfo->getVariableInfo(TODVariableInfo::PARAM_ROLE_AMBIENT_BLEND_THRESHOLD, varInfo);
			paramInfo.mPBRAmbientBlendThreshold = varInfo.fValue[0];
		}

		//-----------------------------------------------------------------------------
		{
			pInfo->getVariableInfo(TODVariableInfo::PARAM_SKY_LIGHT_COLOR_POWER, varInfo);
			fMultiplier = varInfo.fValue[0];

			if (bEnableHDR)
			{
				pInfo->getVariableInfo(TODVariableInfo::PARAM_SKY_LIGHT_HDR_POWER, varInfo);
				fMultiplier *= varInfo.fValue[0];
			}

			pInfo->getVariableInfo(TODVariableInfo::PARAM_SKY_LIGHT_COLOR, varInfo);
			realColor = ColorValue(varInfo.fValue[0], varInfo.fValue[1], varInfo.fValue[2]);
			paramInfo.mSkyLightColor = (ColorValue::gammaToLinear(realColor) * fMultiplier);
		}
		//-----------------------------------------------------------------------------
		{
			pInfo->getVariableInfo(TODVariableInfo::PARAM_SKY_LIGHT_COLOR_POWER, varInfo);
			fMultiplier = varInfo.fValue[0];

			if (bEnableHDR)
			{
				pInfo->getVariableInfo(TODVariableInfo::PARAM_SKY_LIGHT_HDR_POWER, varInfo);
				fMultiplier *= varInfo.fValue[0];
			}

			pInfo->getVariableInfo(TODVariableInfo::PARAM_SKY_LIGHT_COLOR, varInfo);
			realColor = ColorValue(varInfo.fValue[0], varInfo.fValue[1], varInfo.fValue[2]);
			paramInfo.mSkyLightColor = (ColorValue::gammaToLinear(realColor) * fMultiplier);
		}

		//-----------------------------------------------------------------------------
		{
			pInfo->getVariableInfo(TODVariableInfo::PARAM_PBR_SKY_LIGHT_COLOR_POWER, varInfo);
			fMultiplier = varInfo.fValue[0];

			if (bEnableHDR)
			{
				pInfo->getVariableInfo(TODVariableInfo::PARAM_PBR_SKY_LIGHT_HDR_POWER, varInfo);
				fMultiplier *= varInfo.fValue[0];
			}

			pInfo->getVariableInfo(TODVariableInfo::PARAM_PBR_SKY_LIGHT_COLOR, varInfo);
			realColor = ColorValue(varInfo.fValue[0], varInfo.fValue[1], varInfo.fValue[2]);
			paramInfo.mPBRSkyLightColor = (ColorValue::gammaToLinear(realColor) * fMultiplier);
		}

		//-----------------------------------------------------------------------------
		{
			pInfo->getVariableInfo(TODVariableInfo::PARAM_ROLE_SKY_LIGHT_COLOR_POWER, varInfo);
			fMultiplier = varInfo.fValue[0];

			if (bEnableHDR)
			{
				pInfo->getVariableInfo(TODVariableInfo::PARAM_ROLE_SKY_LIGHT_HDR_POWER, varInfo);
				fMultiplier *= varInfo.fValue[0];
			}

			pInfo->getVariableInfo(TODVariableInfo::PARAM_ROLE_SKY_LIGHT_COLOR, varInfo);
			realColor = ColorValue(varInfo.fValue[0], varInfo.fValue[1], varInfo.fValue[2]);
			paramInfo.mRoleSkyLightColor = (ColorValue::gammaToLinear(realColor) * fMultiplier);
		}

		//-----------------------------------------------------------------------------
		{
			pInfo->getVariableInfo(TODVariableInfo::PARAM_GROUND_LIGHT_COLOR_POWER, varInfo);
			fMultiplier = varInfo.fValue[0];

			if (bEnableHDR)
			{
				pInfo->getVariableInfo(TODVariableInfo::PARAM_GROUND_LIGHT_HDR_POWER, varInfo);
				fMultiplier *= varInfo.fValue[0];
			}

			pInfo->getVariableInfo(TODVariableInfo::PARAM_GROUND_LIGHT_COLOR, varInfo);
			realColor = ColorValue(varInfo.fValue[0], varInfo.fValue[1], varInfo.fValue[2]);
			paramInfo.mGroundLightColor = (ColorValue::gammaToLinear(realColor) * fMultiplier);
		}

		//-----------------------------------------------------------------------------
		{
			pInfo->getVariableInfo(TODVariableInfo::PARAM_PBR_GROUND_LIGHT_COLOR_POWER, varInfo);
			fMultiplier = varInfo.fValue[0];

			if (bEnableHDR)
			{
				pInfo->getVariableInfo(TODVariableInfo::PARAM_PBR_GROUND_LIGHT_HDR_POWER, varInfo);
				fMultiplier *= varInfo.fValue[0];
			}

			pInfo->getVariableInfo(TODVariableInfo::PARAM_PBR_GROUND_LIGHT_COLOR, varInfo);
			realColor = ColorValue(varInfo.fValue[0], varInfo.fValue[1], varInfo.fValue[2]);

			paramInfo.mPBRGroundLightColor = (ColorValue::gammaToLinear(realColor) * fMultiplier);
		}

		//-----------------------------------------------------------------------------
		{
			pInfo->getVariableInfo(TODVariableInfo::PARAM_ROLE_GROUND_LIGHT_COLOR_POWER, varInfo);
			fMultiplier = varInfo.fValue[0];

			if (bEnableHDR)
			{
				pInfo->getVariableInfo(TODVariableInfo::PARAM_ROLE_GROUND_LIGHT_HDR_POWER, varInfo);
				fMultiplier *= varInfo.fValue[0];
			}

			pInfo->getVariableInfo(TODVariableInfo::PARAM_ROLE_GROUND_LIGHT_COLOR, varInfo);
			realColor = ColorValue(varInfo.fValue[0], varInfo.fValue[1], varInfo.fValue[2]);
			paramInfo.mRoleGroundLightColor = (ColorValue::gammaToLinear(realColor) * fMultiplier);
		}

		//-----------------------------------------------------------------------------
		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_0_COLOR_POWER, varInfo);
		fMultiplier = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_0_COLOR, varInfo);
		paramInfo.mFogNearColor[0] = (fMultiplier * ColorValue(varInfo.fValue[0],
			varInfo.fValue[1], varInfo.fValue[2]));

		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_0_COLOR_POWER2, varInfo);
		fMultiplier = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_0_COLOR2, varInfo);
		paramInfo.mFogFarColor[0] = (fMultiplier * ColorValue(varInfo.fValue[0],
			varInfo.fValue[1], varInfo.fValue[2]));

		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_1_COLOR_POWER, varInfo);
		fMultiplier = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_1_COLOR, varInfo);
		paramInfo.mFogNearColor[1] = (fMultiplier * ColorValue(varInfo.fValue[0],
			varInfo.fValue[1], varInfo.fValue[2]));

		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_1_COLOR_POWER2, varInfo);
		fMultiplier = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_1_COLOR2, varInfo);
		paramInfo.mFogFarColor[1] = (fMultiplier * ColorValue(varInfo.fValue[0],
			varInfo.fValue[1], varInfo.fValue[2]));

		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_2_COLOR_POWER, varInfo);
		fMultiplier = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_2_COLOR, varInfo);
		paramInfo.mFogNearColor[2] = (fMultiplier * ColorValue(varInfo.fValue[0],
			varInfo.fValue[1], varInfo.fValue[2]));

		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_2_COLOR_POWER2, varInfo);
		fMultiplier = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_2_COLOR2, varInfo);
		paramInfo.mFogFarColor[2] = (fMultiplier * ColorValue(varInfo.fValue[0],
			varInfo.fValue[1], varInfo.fValue[2]));

		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_3_COLOR_POWER, varInfo);
		fMultiplier = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_3_COLOR, varInfo);
		paramInfo.mFogNearColor[3] = (fMultiplier * ColorValue(varInfo.fValue[0],
			varInfo.fValue[1], varInfo.fValue[2]));

		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_3_COLOR_POWER2, varInfo);
		fMultiplier = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_3_COLOR2, varInfo);
		paramInfo.mFogFarColor[3] = (fMultiplier * ColorValue(varInfo.fValue[0],
			varInfo.fValue[1], varInfo.fValue[2]));

		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_0_GLOBAL_DENSITY, varInfo);
		paramInfo.mFogNearDensity[0] = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_1_GLOBAL_DENSITY, varInfo);
		paramInfo.mFogNearDensity[1] = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_2_GLOBAL_DENSITY, varInfo);
		paramInfo.mFogNearDensity[2] = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_3_GLOBAL_DENSITY, varInfo);
		paramInfo.mFogNearDensity[3] = varInfo.fValue[0];

		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_0_GLOBAL_DENSITY2, varInfo);
		paramInfo.mFogFarDensity[0] = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_1_GLOBAL_DENSITY2, varInfo);
		paramInfo.mFogFarDensity[1] = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_2_GLOBAL_DENSITY2, varInfo);
		paramInfo.mFogFarDensity[2] = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_3_GLOBAL_DENSITY2, varInfo);
		paramInfo.mFogFarDensity[3] = varInfo.fValue[0];

		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_0_HEIGHT, varInfo);
		paramInfo.mFogNearTopHeight[0] = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_1_HEIGHT, varInfo);
		paramInfo.mFogNearTopHeight[1] = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_2_HEIGHT, varInfo);
		paramInfo.mFogNearTopHeight[2] = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_3_HEIGHT, varInfo);
		paramInfo.mFogNearTopHeight[3] = varInfo.fValue[0];

		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_0_HEIGHT2, varInfo);
		paramInfo.mFogFarTopHeight[0] = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_1_HEIGHT2, varInfo);
		paramInfo.mFogFarTopHeight[1] = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_2_HEIGHT2, varInfo);
		paramInfo.mFogFarTopHeight[2] = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_3_HEIGHT2, varInfo);
		paramInfo.mFogFarTopHeight[3] = varInfo.fValue[0];

		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_0_RAMP_START, varInfo);
		paramInfo.mFogNearStart[0] = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_0_RAMP_START2, varInfo);
		paramInfo.mFogFarStart[0] = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_1_RAMP_START, varInfo);
		paramInfo.mFogNearStart[1] = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_1_RAMP_START2, varInfo);
		paramInfo.mFogFarStart[1] = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_2_RAMP_START, varInfo);
		paramInfo.mFogNearStart[2] = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_2_RAMP_START2, varInfo);
		paramInfo.mFogFarStart[2] = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_3_RAMP_START, varInfo);
		paramInfo.mFogNearStart[3] = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_3_RAMP_START2, varInfo);
		paramInfo.mFogFarStart[3] = varInfo.fValue[0];

		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_0_RAMP_END, varInfo);
		paramInfo.mFogNearEnd[0] = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_0_RAMP_END2, varInfo);
		paramInfo.mFogFarEnd[0] = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_1_RAMP_END, varInfo);
		paramInfo.mFogNearEnd[1] = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_1_RAMP_END2, varInfo);
		paramInfo.mFogFarEnd[1] = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_2_RAMP_END, varInfo);
		paramInfo.mFogNearEnd[2] = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_2_RAMP_END2, varInfo);
		paramInfo.mFogFarEnd[2] = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_3_RAMP_END, varInfo);
		paramInfo.mFogNearEnd[3] = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_FOG_3_RAMP_END2, varInfo);
		paramInfo.mFogFarEnd[3] = varInfo.fValue[0];

		pInfo->getVariableInfo(TODVariableInfo::PARAM_SHADOW_WEIGHT, varInfo);
		paramInfo.mShadowWeight = varInfo.fValue[0];

		pInfo->getVariableInfo(TODVariableInfo::PARAM_GLOBAL_POINT_LIGHT_POWER, varInfo);
		paramInfo.mGlobalPointLightPower = varInfo.fValue[0];

		pInfo->getVariableInfo(TODVariableInfo::PARAM_ROLE_POINT_LIGHT_POWER, varInfo);
		paramInfo.mRolePointLightPower = varInfo.fValue[0];

		pInfo->getVariableInfo(TODVariableInfo::PARAM_ROLE_POINT_LIGHT_COLOR, varInfo);
		paramInfo.mRolePointLightColor = ColorValue(varInfo.fValue[0],
			varInfo.fValue[1], varInfo.fValue[2]);

		pInfo->getVariableInfo(TODVariableInfo::PARAM_GLOBAL_SPECULAR_RATIO, varInfo);
		paramInfo.mGlobalSpecularRatio = varInfo.fValue[0];

		pInfo->getVariableInfo(TODVariableInfo::PARAM_GLOBAL_ENVIRONMENT_EXPLOSURE, varInfo);
		paramInfo.mGlobalEnvironmentExplosure = varInfo.fValue[0];

		pInfo->getVariableInfo(TODVariableInfo::PARAM_GLOBAL_REFLECTION_RATIO, varInfo);
		paramInfo.mGlobalReflectionRatio = varInfo.fValue[0];

		pInfo->getVariableInfo(TODVariableInfo::PARAM_GLOBAL_CORRECTION_COLOR, varInfo);
		paramInfo.mGlobalCorrectionColor = ColorValue(varInfo.fValue[0],
			varInfo.fValue[1], varInfo.fValue[2]);

		pInfo->getVariableInfo(TODVariableInfo::PARAM_GLOBAL_VEG_CORRECTION_COLOR, varInfo);
		paramInfo.mGlobalVegCorrectionColor = ColorValue(varInfo.fValue[0],
			varInfo.fValue[1], varInfo.fValue[2]);

		pInfo->getVariableInfo(TODVariableInfo::PARAM_GLOBAL_BUILD_CORRECTION_COLOR, varInfo);
		paramInfo.mGlobalBuildCorrectionColor = ColorValue(varInfo.fValue[0],
			varInfo.fValue[1], varInfo.fValue[2]);

		pInfo->getVariableInfo(TODVariableInfo::PARAM_GLOBAL_TERRAIN_CORRECTION_COLOR, varInfo);
		paramInfo.mGlobalTerrainCorrectionColor = ColorValue(varInfo.fValue[0],
			varInfo.fValue[1], varInfo.fValue[2]);


		pInfo->getVariableInfo(TODVariableInfo::PARAM_POST_PROCESS_COLOR_GRADING, varInfo);
		paramInfo.mColorGradingTextureName = varInfo.sValue;

		pInfo->getVariableInfo(TODVariableInfo::PARAM_POST_PROCESS_BLOOM_THRESHOLD, varInfo);
		paramInfo.mBloomThreshold = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_POST_PROCESS_BLOOM_BLUR_STRENGTH, varInfo);
		paramInfo.mBloomBlurStrength = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_POST_PROCESS_BLOOM_BLUR_RADIUS, varInfo);
		paramInfo.mBloomBlurRadius = varInfo.fValue[0];

		pInfo->getVariableInfo(TODVariableInfo::PARAM_POST_PROCESS_HDR_EYEADAPTATION_0X, varInfo);
		paramInfo.mHDREyeParam[0] = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_POST_PROCESS_HDR_EYEADAPTATION_0Y, varInfo);
		paramInfo.mHDREyeParam[1] = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_POST_PROCESS_HDR_EYEADAPTATION_0Z, varInfo);
		paramInfo.mHDREyeParam[2] = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_POST_PROCESS_HDR_EYEADAPTATION_0W, varInfo);
		paramInfo.mHDREyeParam[3] = varInfo.fValue[0];

		pInfo->getVariableInfo(TODVariableInfo::PARAM_POST_PROCESS_HDR_BRIGHT_0X, varInfo);
		paramInfo.mHDRBrightParam[0] = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_POST_PROCESS_HDR_BRIGHT_0Y, varInfo);
		paramInfo.mHDRBrightParam[1] = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_POST_PROCESS_HDR_BRIGHT_0Z, varInfo);
		paramInfo.mHDRBrightParam[2] = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_POST_PROCESS_HDR_BRIGHT_0W, varInfo);
		paramInfo.mHDRBrightParam[3] = varInfo.fValue[0];

		pInfo->getVariableInfo(TODVariableInfo::PARAM_POST_PROCESS_HDR_TONEMAPPING_0X, varInfo);
		paramInfo.mHDRToneMappingParam[0] = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_POST_PROCESS_HDR_TONEMAPPING_0Y, varInfo);
		paramInfo.mHDRToneMappingParam[1] = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_POST_PROCESS_HDR_TONEMAPPING_0Z, varInfo);
		paramInfo.mHDRToneMappingParam[2] = varInfo.fValue[0];

		pInfo->getVariableInfo(TODVariableInfo::PARAM_POST_PROCESS_HDR_TONEMAPPING_2X, varInfo);
		paramInfo.mHDRToneMappingCurve[0] = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_POST_PROCESS_HDR_TONEMAPPING_2Y, varInfo);
		paramInfo.mHDRToneMappingCurve[1] = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_POST_PROCESS_HDR_TONEMAPPING_2Z, varInfo);
		paramInfo.mHDRToneMappingCurve[2] = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_POST_PROCESS_HDR_TONEMAPPING_2W, varInfo);
		paramInfo.mHDRToneMappingCurve[3] = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_POST_PROCESS_HDR_TONEMAPPING_0W, varInfo);
		paramInfo.mHDRToneMappingCurve[4] = varInfo.fValue[0];

		pInfo->getVariableInfo(TODVariableInfo::PARAM_POST_PROCESS_HDR_TONEMAPPING_3, varInfo);
		paramInfo.mHDRToneMappingColor[0] = varInfo.fValue[0];
		paramInfo.mHDRToneMappingColor[1] = varInfo.fValue[1];
		paramInfo.mHDRToneMappingColor[2] = varInfo.fValue[2];

		pInfo->getVariableInfo(TODVariableInfo::PARAM_POST_PROCESS_VOLUME_LIGHT_AMOUNT, varInfo);
		paramInfo.mVolumeLightAmount = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_POST_PROCESS_VOLUME_LIGHT__ATTENUATION, varInfo);
		paramInfo.mVolumeLightAttenuation = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_POST_PROCESS_VOLUME_LIGHT__COLORINTENSITY, varInfo);
		paramInfo.mVolumeLightColorIntensity = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_POST_PROCESS_VOLUME_LIGHT_COLOR, varInfo);
		paramInfo.mVolumeLightColor[0] = varInfo.fValue[0];
		paramInfo.mVolumeLightColor[1] = varInfo.fValue[1];
		paramInfo.mVolumeLightColor[2] = varInfo.fValue[2];

		pInfo->getVariableInfo(TODVariableInfo::PARAM_POST_PROCESS_SSAO_INTENSITY, varInfo);
		paramInfo.mSSAOIntensity = varInfo.fValue[0];

		pInfo->getVariableInfo(TODVariableInfo::PARAM_WEATHER_COVER_COLOR, varInfo);
		paramInfo.mWeatherCoverColor = ColorValue(varInfo.fValue[0],
			varInfo.fValue[1], varInfo.fValue[2]);
		pInfo->getVariableInfo(TODVariableInfo::PARAM_WEATHER_COVER_COLOR_POWER, varInfo);
		paramInfo.mCoverColorIntensity = varInfo.fValue[0];
		//-----------------------------------------------------------------------------
		pInfo->getVariableInfo(TODVariableInfo::PARAM_OCEAN_WIND_POWER, varInfo);
		paramInfo.mOceanWindPower = varInfo.fValue[0];

		pInfo->getVariableInfo(TODVariableInfo::PARAM_OCEAN_SHALLOW_COLOR, varInfo);
		paramInfo.mOceanShallowColor = ColorValue(varInfo.fValue[0],
			varInfo.fValue[1], varInfo.fValue[2]);

		pInfo->getVariableInfo(TODVariableInfo::PARAM_OCEAN_DEEP_COLOR, varInfo);
		paramInfo.mOceanDeepColor = ColorValue(varInfo.fValue[0],
			varInfo.fValue[1], varInfo.fValue[2]);

		pInfo->getVariableInfo(TODVariableInfo::PARAM_OCEAN_FOAM_COLOR, varInfo);
		paramInfo.mOceanFoamColor = ColorValue(varInfo.fValue[0],
			varInfo.fValue[1], varInfo.fValue[2]);

		pInfo->getVariableInfo(TODVariableInfo::PARAM_OCEAN_REFLECT_COLOR, varInfo);
		paramInfo.mOceanReflectColor = ColorValue(varInfo.fValue[0],
			varInfo.fValue[1], varInfo.fValue[2]);

		pInfo->getVariableInfo(TODVariableInfo::PARAM_OCEAN_UNDERWATER_FOG_COLOR, varInfo);
		paramInfo.mOceanUnderWaterFogColor = ColorValue(varInfo.fValue[0],
			varInfo.fValue[1], varInfo.fValue[2]);

		pInfo->getVariableInfo(TODVariableInfo::PARAM_OCEAN_UNDERWATER_OCEAN_COLOR, varInfo);
		paramInfo.mOceanUnderWaterOceanColor = ColorValue(varInfo.fValue[0],
			varInfo.fValue[1], varInfo.fValue[2]);

		pInfo->getVariableInfo(TODVariableInfo::PARAM_UNDERWATER_CAUSTIC_COLOR_POWER, varInfo);
		float causticColorPower = varInfo.fValue[0];
		pInfo->getVariableInfo(TODVariableInfo::PARAM_UNDERWATER_CAUSTIC_COLOR, varInfo);
		paramInfo.mOceanUnderWaterCausticColor = ColorValue(varInfo.fValue[0],
			varInfo.fValue[1], varInfo.fValue[2], causticColorPower);

		pInfo->getVariableInfo(TODVariableInfo::PARAM_WIND_POWER, varInfo);
		paramInfo.mWindPower = varInfo.fValue[0];

		pInfo->getVariableInfo(TODVariableInfo::PARAM_WIND_ANGLE, varInfo);
		paramInfo.mWindAngle = varInfo.fValue[0];

		pInfo->getVariableInfo(TODVariableInfo::PARAM_WIND_TREE_SWAY_AMP, varInfo);
		paramInfo.mWindTreeSwayAmp = varInfo.fValue[0];

		pInfo->getVariableInfo(TODVariableInfo::PARAM_WIND_TREE_SWAY_FREQ, varInfo);
		paramInfo.mWindTreeSwayFreq = varInfo.fValue[0];

		pInfo->getVariableInfo(TODVariableInfo::PARAM_WIND_TREE_SHAKE_AMP, varInfo);
		paramInfo.mWindTreeShakeAmp = varInfo.fValue[0];

		pInfo->getVariableInfo(TODVariableInfo::PARAM_WIND_TREE_SHAKE_FREQ, varInfo);
		paramInfo.mWindTreeShakeFreq = varInfo.fValue[0];

		pInfo->getVariableInfo(TODVariableInfo::PARAM_WIND_VEG_SWAY_AMP, varInfo);
		paramInfo.mWindVegSwayAmp = varInfo.fValue[0];

		pInfo->getVariableInfo(TODVariableInfo::PARAM_WIND_VEG_SWAY_FREQ, varInfo);
		paramInfo.mWindVegSwayFreq = varInfo.fValue[0];

		pInfo->getVariableInfo(TODVariableInfo::PARAM_WIND_VEG_SHAKE_AMP, varInfo);
		paramInfo.mWindVegShakeAmp = varInfo.fValue[0];

		pInfo->getVariableInfo(TODVariableInfo::PARAM_WIND_VEG_SHAKE_FREQ, varInfo);
		paramInfo.mWindVegShakeFreq = varInfo.fValue[0];

		pInfo->getVariableInfo(TODVariableInfo::PARAM_ATMOS_ATMOSPHERE_HEIGHT, varInfo);
		paramInfo.mAtmosphereHeight = varInfo.fValue[0];

		pInfo->getVariableInfo(TODVariableInfo::PARAM_ATMOS_ATMOSPHERE_BLEND_RANGE, varInfo);
		paramInfo.mAtmosphereBlendRange = varInfo.fValue[0];

		pInfo->getVariableInfo(TODVariableInfo::PARAM_ATMOS_LOWER_LAYER_COLOR, varInfo);
		paramInfo.mAtmosphereLowerLayerColor = ColorValue(varInfo.fValue[0], varInfo.fValue[1], varInfo.fValue[2], 0.f);

		pInfo->getVariableInfo(TODVariableInfo::PARAM_ATMOS_UPPER_LAYER_COLOR, varInfo);
		paramInfo.mAtmosphereUpperLayerColor = ColorValue(varInfo.fValue[0], varInfo.fValue[1], varInfo.fValue[2], 0.f);

		pInfo->getVariableInfo(TODVariableInfo::PARAM_ATMOS_TRANSMITTANCE_EXP_SCALE, varInfo);
		paramInfo.mAtmosphereTransmittanceExpScale = varInfo.fValue[0];

		pInfo->getVariableInfo(TODVariableInfo::PARAM_ATMOS_TRANSMITTANCE_EXP_TERM, varInfo);
		paramInfo.mAtmosphereTransmittanceExpTerm = varInfo.fValue[0];

		pInfo->getVariableInfo(TODVariableInfo::PARAM_ATMOS_MAX_ALPHA, varInfo);
		paramInfo.mAtmosphereMaxAlpha = varInfo.fValue[0];

		//可见光分量
		pInfo->getVariableInfo(TODVariableInfo::PARAM_VIEW_LIGHT_PERCENT, varInfo);
		paramInfo.mViewLightPercent[0] = varInfo.fValue[0];

		pInfo->getVariableInfo(TODVariableInfo::PARAM_PBR_VIEW_LIGHT_PERCENT, varInfo);
		paramInfo.mViewLightPercent[1] = varInfo.fValue[0];

		pInfo->getVariableInfo(TODVariableInfo::PARAM_ROLE_VIEW_LIGHT_PERCENT, varInfo);
		paramInfo.mViewLightPercent[2] = varInfo.fValue[0];
	}

	const std::vector<Echo::TODTimeSpeedSlot>& TODManager::getTODTimeSpeedSlots() const
	{
		EchoZoneScoped;

		return mTimeSpeedSlots;
	}

	void TODManager::setTODTimeSpeedSlots(const std::vector<TODTimeSpeedSlot>& speedSlots)
	{
		EchoZoneScoped;

		mTimeSpeedSlots.clear();
		mTimeSpeedSlots = speedSlots;
		_updateTimePeriodSpeed();
	}

	void TODManager::setTimePassed(int time)
	{
		mTimePassed = time;
	}

	void TODManager::setGlobalTimeSpeedSlots(const std::vector<TODTimeSpeedSlot>& speedSlots)
	{
		EchoZoneScoped;

		mGlobalTimeSpeedSlots.clear();
		mGlobalTimeSpeedSlots = speedSlots;
	}

	void TODManager::useGlobalTimeSpeedSlots()
	{
		EchoZoneScoped;

		setTODTimeSpeedSlots(mGlobalTimeSpeedSlots);
	}

	double TODManager::getTimeSpeed() const
	{
		EchoZoneScoped;

		DEF_THREAD_CHEEK(DEF_THREAD_TYPE_ALL);

		return mTimeSpeed;
		//return mTimeSpeed * 24.0 * 3600.0;
	}

	void TODManager::setTimeSpeed(double s)
	{
		EchoZoneScoped;

		DEF_THREAD_CHEEK(DEF_THREAD_TYPE_ALL);

		mTimeSpeed = s;
		//mTimeSpeed = s / 24.0 / 3600.0;

		if (mPlayEnable || mSrcPlayEnable)
		{
			mCurrentTime = calculateCurTodTime();
		}
	}

	void TODManager::setTimeStep(double s)
	{
		EchoZoneScoped;

		mTimeStep = s / 60.0 / 24.0;
	}

	void TODManager::setMainLightPitchLimit(float s)
	{
		EchoZoneScoped;

		mMainLightPitchLimit = s;
	}

	float TODManager::getMainLightPitchLimit()
	{
		EchoZoneScoped;

		return mMainLightPitchLimit;
	}

	double TODManager::getTimeStep() const
	{
		EchoZoneScoped;

		return mTimeStep * 60.0 * 24.0;
	}

	TODVariableInfo* TODManager::createTod(const String& name)
	{
		EchoZoneScoped;

		removeTod(name);
		TODVariableInfo* tod = new TODVariableInfo(this);
		mTodMap[name] = tod;
		for (TODManagerListenners::iterator it = mListenners.begin(); it != mListenners.end(); ++it)
		{
			TODManagerListenner* l = *it;
			if (l)
			{
				l->onTodListChanged(mTodMap);
			}
		}
		selectTod(name);
		return tod;
	}

	bool TODManager::saveTod(const String& name)
	{
		EchoZoneScoped;

		TODVariableInfo* pSaveVariableInfo = nullptr;
		TODMap::iterator itfind = mTodMap.find(name);
		if (itfind == mTodMap.end())
			return false;

		pSaveVariableInfo = itfind->second;
		String path = mRootPath + name;
		return TODSerializer::saveTodFile(path, pSaveVariableInfo);
	}

	bool TODManager::saveTodAs(const String& name, const String& sNewPathFull)
	{
		EchoZoneScoped;

		TODVariableInfo* pSaveVariableInfo = nullptr;
		TODMap::iterator itfind = mTodMap.find(name);
		if (itfind == mTodMap.end())
			return false;

		pSaveVariableInfo = itfind->second;
		return TODSerializer::saveTodFile(sNewPathFull, pSaveVariableInfo);
	}
	//void TODManager::SaveTod( const String& name,const AreaParameters& params )
	//{
	//	CreateTod(name,params);
	//	ParamTraceMap *todp=mTodMap[name];
	//	String path=mTodFileDir+name;
	//	TODSerializer::getSingleton()->SaveTodFile(path,*todp);
	//}

	String TODManager::getCurrentTodName() const
	{
		EchoZoneScoped;

		DEF_THREAD_CHEEK(DEF_THREAD_TYPE_ALL);

		for (TODMap::const_iterator it = mTodMap.begin(); it != mTodMap.end(); ++it)
		{
			if (mCurrentVariableInfo == it->second)
			{
				return it->first;
			}
		}
		return "";
	}

	TODVariableInfo* TODManager::getCurrentTODVariableInfo() const
	{
		EchoZoneScoped;

		return mCurrentVariableInfo;
	}

	void TODManager::setTodFileDir(const String& dir)
	{
		EchoZoneScoped;

		mTodFileDir = dir;
	}

	const String& TODManager::getTodFileDir() const
	{
		EchoZoneScoped;

		return mTodFileDir;
	}

	const String& TODManager::getRootPath() const
	{
		EchoZoneScoped;

		return mRootPath;
	}

	bool TODManager::isMoonRise() const
	{
		EchoZoneScoped;

		return mMoonRise;
	}

	void TODManager::createTod(const String& todName, const vector<VariableValue>::type& params)
	{
		EchoZoneScoped;

		removeTod(todName);
		TODVariableInfo* tod = new TODVariableInfo(this);
		mTodMap[todName] = tod;
		if (!mCurrentVariableInfo)
		{
			mCurrentVariableInfo = tod;
		}

		vector<VariableValue>::const_iterator it, ite = params.end();
		for (it = params.begin(); it != ite; ++it)
		{
			tod->setVariableValue(it->typeId, it->fValue);
		}

		for (TODManagerListenners::iterator itor = mListenners.begin(); itor != mListenners.end(); ++itor)
		{
			TODManagerListenner* l = *itor;
			if (l)
			{
				l->onTodListChanged(mTodMap);
			}
		}
	}

	void TODManager::enter(const String& tod, float time, TODPriorityLevel lvl)
	{
		EchoZoneScoped;

		DEF_THREAD_CHEEK(DEF_THREAD_TYPE_ALL);

		if (time <= 0.0f)
		{
			mTranslationTime = 0.01f;
			mElapseTime = 0.0f;
		}
		else
			mTranslationTime = time;

		if (!selectTod(tod, false))
		{
			mElapseTime = mTranslationTime;
		}
		else
		{
			mTODPriorityLevel = lvl;
			mElapseTime = 0.0f;
			mCommonTodName = tod;
		}
	}

	//void TODManager::enter(const String& tod, 
	//	const String& raintod,float time, TODPriorityLevel lvl)
	//{
	//	mCommonTodName = tod;
	//	if (loadTod(raintod))
	//		mRainTodName = raintod;
	//	WeatherManager::WeatherType type = getWeatherManager()->getWeatherType();
	//	if (type == WeatherManager::WT_RAIN)
	//	{
	//		enter(mRainTodName, time,lvl);
	//	}
	//	else
	//	{
	//		enter(tod, time,lvl);
	//	}
	//}

	template<class T>
	T TODManager::lerp(const T& t1, const T& t2, float factor)
	{
		EchoZoneScoped;

		return t1 + factor * (t2 - t1);
	}

	ColorValue TODManager::lerp(const ColorValue& clr1, const ColorValue& clr2, float factor)
	{
		EchoZoneScoped;

		return ColorValue(lerp(clr1.r, clr2.r, factor),
			lerp(clr1.g, clr2.g, factor),
			lerp(clr1.b, clr2.b, factor),
			lerp(clr1.a, clr2.a, factor));
	}

	void TODManager::setUseCustomColorChart(bool value, const String& colorChartName)
	{
		EchoZoneScoped;

		DEF_THREAD_CHEEK(DEF_THREAD_TYPE_ALL);

		if (mUseCustomColorChart == value)
			return;

		if (value)
		{
			mCustomColorGradingTexturePtr = TextureManager::instance()->loadTexture(Name(colorChartName), TEX_TYPE_2D);
			if (!mCustomColorGradingTexturePtr.isNull())
			{
				mUseCustomColorChart = value;
				updateData(0.0, true);
			}
		}
		else
		{
			mCustomColorGradingTexturePtr.setNull();
			mUseCustomColorChart = value;
			mCurrentColorChartName = "Default";
			updateData(0.0, true);
		}

		if (Root::instance()->getRuntimeMode() == Root::eRM_Editor)
			ProtocolComponentSystem::instance()->callProtocolFunc(ProtocolFuncID::PFID_ResetTodComAttribute, &mName, 0);
	}

	bool TODManager::isUseCustomColorChart() const
	{
		EchoZoneScoped;

		DEF_THREAD_CHEEK(DEF_THREAD_TYPE_ALL);
		return mUseCustomColorChart;
	}

	bool TODManager::frameStarted(const FrameEvent& frameEvent)
	{
		EchoZoneScoped;

		updateData(frameEvent.timeSinceLastFrame, mForceUpdate);
		if (mForceUpdate)	// 修改屏幕缩放会强制刷新属性
		{
			ProtocolComponentSystem::instance()->callProtocolFunc(ProtocolFuncID::PFID_ResetTodComAttribute, &mName, 0);
		}
		mForceUpdate = false;
		return true;
	}

	void TODManager::OnDeivceReset()
	{
		EchoZoneScoped;

		mForceUpdate = true;
		//updateData(0.0, true);
	}

	void TODManager::ResizeEvent(uint32 width, uint32 height)
	{
		EchoZoneScoped;

		mForceUpdate = true;
		//updateData(0.0, true);
	}

	void TODManager::OnInitRenderLister(RenderSystem* renderSys)
	{
		EchoZoneScoped;

		renderSys->addRenderListener(this);
		renderSys->addResizeListener(this);
	}

	void TODManager::UnInitRenderLister(RenderSystem* renderSys)
	{
		EchoZoneScoped;

		renderSys->removeRenderListener(this);
		renderSys->removeResizeListener(this);
	}

	void TODManager::enablePlay(bool b)
	{
		EchoZoneScoped;

		DEF_THREAD_CHEEK(DEF_THREAD_TYPE_ALL);
		enablePlay(b, mSrcStaticTime);
	}

	void TODManager::enablePlay(bool b, double t)
	{
		EchoZoneScoped;

		DEF_THREAD_CHEEK(DEF_THREAD_TYPE_ALL);

		mPlayEnable = b;

		if (!b)
		{
			mStaticTime = std::min(std::max(t, 0.0), 1.0);
			mElapseTime = 0.0f;
		}
		else
		{
			mStaticTime = mSrcStaticTime;
		}
	}

	void TODManager::setSrcStaticTime(double t)
	{
		EchoZoneScoped;

		mSrcStaticTime = std::min(std::max(t, 0.0), 1.0);
	}

	double TODManager::getSrcStaticTime() const
	{
		EchoZoneScoped;

		return mSrcStaticTime;
	}

	void TODManager::setSrcEnablePlay(bool b)
	{
		EchoZoneScoped;

		mSrcPlayEnable = b;
	}

	bool TODManager::getSrcEnablePlay() const
	{
		EchoZoneScoped;

		return mSrcPlayEnable;
	}

	void TODManager::restore()
	{
		EchoZoneScoped;

		enableSrcPlay(mSrcPlayEnable, mSrcStaticTime);
		updateData(0.0, true);
	}

	void TODManager::addListenner(TODManagerListenner* l)
	{
		EchoZoneScoped;

		if (l)
		{
			mListenners.push_back(l);
			l->onTodListChanged(mTodMap);
		}
	}

	void TODManager::removeListenner(TODManagerListenner* l)
	{
		EchoZoneScoped;

		if (l)
			mListenners.remove(l);
	}

	void TODManager::clearListenner()
	{
		EchoZoneScoped;

		mListenners.clear();
	}

	void TODManager::enableSrcPlay(bool b, double t)
	{
		EchoZoneScoped;

		if (b)
		{
			mCurrentTime = calculateCurTodTime();
		}
		enablePlay(b, t);
	}

	void TODManager::setSvrTime(uint64 dwSvrTime, uint32 dwDayTime)
	{
		EchoZoneScoped;

		DEF_THREAD_CHEEK(DEF_THREAD_TYPE_ALL);

		m_dwSvrTime = dwSvrTime;
		m_dwDayTime = dwDayTime;
		m_currentTotalTime = 0.0;
		m_currentRealTotalTime = 0.0;
#ifdef _WIN32
		m_dwTickTime = GetTickCount();
#else
		struct timeval tv;
		gettimeofday(&tv, 0);
		m_dwTickTime = tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
		if (mPlayEnable || mSrcPlayEnable)
		{
			mCurrentTime = calculateCurTodTime();
			enablePlay(mPlayEnable);
		}
	}

	double TODManager::calculateCurTodTime()
	{
		EchoZoneScoped;

		double t = 0.0;

		if (0 == m_dwDayTime || 0 == m_dwSvrTime)
		{
			return t;
		}

		uint32 moonPhaseIndex = 0;

		uint64 u64Time = m_dwSvrTime + (_getCurrentTick() - m_dwTickTime) / 1000;		//计算服务器传递过来的时间
		if (Root::instance()->getRuntimeMode() != Root::eRM_Editor)
		{
			double dTime = u64Time * getTimeSpeed();
			m_currentTotalTime = std::fmod(dTime, m_dwDayTime);

			moonPhaseIndex = (uint32)(std::fmod(dTime + (double)mTimePassed, m_dwDayTime) / m_dwDayTime);
		}
		else
		{
			m_currentTotalTime = double(u64Time);

			moonPhaseIndex = (uint32)(std::fmod(m_currentTotalTime + (double)mTimePassed, m_dwDayTime) / m_dwDayTime);
		}

		{
			double sec = std::fmod(m_currentTotalTime + (double)mTimePassed + (double)mLocationTimeOffsetSeconds, (double)m_dwDayTime);
			if (sec < 0) sec += m_dwDayTime;
			t = sec / m_dwDayTime;
		}


		double realElapsedtime = 0.0;
		for (size_t i = 0, count = mTimeSpeedSlots.size(); i < count; i++)
		{
			const TODTimeSpeedSlot& slot = mTimeSpeedSlots[i];

			if (slot.mRealStart < t && t <= slot.mRealEnd)
			{
				realElapsedtime += (t - slot.mRealStart) * slot.mSpeed;
				break;
			}
			else if (t > slot.mRealEnd)
			{
				realElapsedtime += (slot.mRealEnd - slot.mRealStart) * slot.mSpeed;
			}
		}

		if (mTimeSpeedSlots.size() > 0)
		{
			m_currentRealTotalTime = realElapsedtime * m_dwDayTime;
			double sec2 = std::fmod(m_currentRealTotalTime + (double)mTimePassed + (double)mLocationTimeOffsetSeconds, (double)m_dwDayTime);
			if (sec2 < 0) sec2 += m_dwDayTime;
			t = sec2 / m_dwDayTime;
		}
		else
		{
			m_currentRealTotalTime = m_currentTotalTime;
		}

		char logInfo[512];
		snprintf(logInfo, 512, "[TOD]\tSvrTime:%lld, Speed:%f, TotalTime:%f, RealTotalTime:%f, curTime:%f, MoonPhase:%u", m_dwSvrTime, mTimeSpeed, m_currentTotalTime, m_currentRealTotalTime, t, (moonPhaseIndex % 10));
		LogManager::getSingleton().logMessage(logInfo, LML_CRITICAL);

		if (mSceneManager)
		{
			Sun* pSun = mSceneManager->getSun();
			if (pSun)
			{
				double curTime = t - (int)t;
				if (curTime < 1e-10)
					curTime = 0.0;

				bool bMoonRise = false;
				if (g_dayFactor < curTime && curTime < g_nightFactor)
				{
					bMoonRise = false;
				}
				else
				{
					if (curTime >= g_nightFactor) //月亮已升起，但还没过0点，需要把月相加1 
						moonPhaseIndex += 1;

					bMoonRise = true;
				}

				pSun->setMoonPhase(moonPhaseIndex, bMoonRise);
				pSun->setMoonRise(bMoonRise);
			}
		}

		return t;
	}

	TODPriorityLevel TODManager::getCurTODPriorityLevel() const
	{
		EchoZoneScoped;

		DEF_THREAD_CHEEK(DEF_THREAD_TYPE_ALL);

		return mTODPriorityLevel;
	}

	void TODManager::setTODPriorityLevel(TODPriorityLevel lvl)
	{
		EchoZoneScoped;

		DEF_THREAD_CHEEK(DEF_THREAD_TYPE_ALL);

		mTODPriorityLevel = lvl;
	}

	double TODManager::getCurrentTime() const
	{
		EchoZoneScoped;

		DEF_THREAD_CHEEK(DEF_THREAD_TYPE_ALL);

		return mCurrentTime;
	}
	//	float TODManager::calculateCurTodTime()
	//	{
	//		float t = 0.0f;
	//
	//		if (0 == m_dwDayTime || 0 == m_dwSvrTime)
	//		{
	//			return t;
	//		}
	//		
	//#ifdef _WIN32
	//		__time64_t u64Time = m_dwSvrTime + (GetTickCount() - m_dwTickTime) / 1000;
	//		if (Root::instance()->getRuntimeMode() != Root::eRM_Editor)
	//		{
	//			struct tm *lotm1 = _localtime64(&u64Time);
	//			uint64 myDayTime = lotm1->tm_hour * 3600 + lotm1->tm_min * 60 + lotm1->tm_sec;
	//			uint64 adjustTime = uint64((u64Time - _getReferenceTime()) * getTimeSpeed());
	//			myDayTime = Math::FloatEqual(getTimeSpeed(),1.0f,0.0001f) ? 0 : myDayTime; //如果Tod速度是1.0，则需要在增量时间上减掉当天已过的秒数
	//			u64Time += adjustTime - myDayTime;
	//		}
	//		struct tm *lotm1 = _localtime64(&u64Time);
	//		uint32 dwTotalSecond = lotm1->tm_hour * 3600 + lotm1->tm_min * 60 + lotm1->tm_sec;
	//		t = float(dwTotalSecond%m_dwDayTime) / float(m_dwDayTime);
	//		m_currentTotalTime = double(dwTotalSecond);
	//#else
	//		struct timeval tv;
	//		gettimeofday(&tv, 0);
	//		unsigned long current_tick = tv.tv_sec * 1000 + tv.tv_usec / 1000;
	//
	//		time_t  u64Time = m_dwSvrTime + (current_tick - m_dwTickTime) / 1000;
	//		if (Root::instance()->getRuntimeMode() != Root::eRM_Editor)
	//		{
	//			struct tm *lotm1 = localtime(&u64Time);
	//			uint64 myDayTime = lotm1->tm_hour * 3600 + lotm1->tm_min * 60 + lotm1->tm_sec;
	//			uint64 adjustTime = (u64Time - _getReferenceTime()) * getTimeSpeed();
	//			myDayTime = Math::FloatEqual(getTimeSpeed(), 1.0f, 0.0001f) ? 0 : myDayTime; //如果Tod速度是1.0，则需要在增量时间上减掉当天已过的秒数
	//			u64Time += adjustTime - myDayTime;
	//		}
	//		struct tm *lotm1 = localtime(&u64Time);
	//		uint32 dwTotalSecond = lotm1->tm_hour * 3600 + lotm1->tm_min * 60 + lotm1->tm_sec;
	//		t = float(dwTotalSecond%m_dwDayTime) / float(m_dwDayTime);
	//		m_currentTotalTime = double(dwTotalSecond);
	//#endif
	//		t = _getRealElapseTime(t);
	//		return t;
	//	}

	void TODManager::getTodStatus(String& commenName, String& rainName, double& staticTime, float& transTime, bool& enablePlay)
	{
		EchoZoneScoped;

		commenName = mCommonTodName;
		rainName = mRainTodName;
		staticTime = mStaticTime;
		transTime = mTranslationTime;
		enablePlay = mPlayEnable;
	}

	bool TODManager::getTodStatusData(TODVariableInfo*& pData)
	{
		EchoZoneScoped;

		if (mCurrentVariableInfo)
		{
			pData = mCurrentVariableInfo;
			return true;
		}
		return false;
	}

	void TODManager::setDefaultColorChart(const String& name)
	{
		EchoZoneScoped;

		bool needChg = false;
		if (!mCurrentColorChartTex.isNull() && (mDefaultColorGradingTexturePtr == mCurrentColorChartTex))
		{
			needChg = true;
		}

		mDefaultColorGradingTexturePtr = TextureManager::instance()->loadTexture(Name(name), TEX_TYPE_2D);

		if (needChg)
		{
			mCurrentColorChartTex = mDefaultColorGradingTexturePtr.getPointer();
		}
	}

	Texture* TODManager::_getDefaultColorChart()
	{
		EchoZoneScoped;

		return mDefaultColorGradingTexturePtr.getPointer();
	}

	Texture* TODManager::_getColorChart(const String& name)
	{
		EchoZoneScoped;

		std::map<String, TexturePtr>::const_iterator it = mColorGradingTextureMap.find(name);
		if (it != mColorGradingTextureMap.end())
		{
			return it->second.getPointer();
		}

		return nullptr;
	}

	Echo::Texture* TODManager::getCurrentColorChart() const
	{
		if (mUseCustomColorChart)
			return mCustomColorGradingTexturePtr.getPointer();

		return mCurrentColorChartTex.getPointer();
	}

	void TODManager::setCurrentColorChart(const String& name)
	{
		EchoZoneScoped;

		TexturePtr tempColorChartPtr;
		if (mUseCustomColorChart)
		{
			tempColorChartPtr = mCustomColorGradingTexturePtr;
		}
		else
		{
			if (mCurrentColorChartName == name)
				return;
			Texture* colorChartTex = _getColorChart(name);
			if (!colorChartTex) {
				colorChartTex = _getDefaultColorChart();
			}
			mCurrentColorChartName = name;
			mCurrentColorChartTex = colorChartTex;

			tempColorChartPtr = mCurrentColorChartTex;
		}

		RenderSystem* pRenderSystem = Root::instance()->getRenderSystem();
		if (!pRenderSystem)
			return;
		RenderStrategy* pStrategy = pRenderSystem->getCurRenderStrategy();
		if (!pStrategy)
			return;
		PostProcessChain* pProcessChain = pStrategy->getPostProcessChain();
		if (!pProcessChain)
			return;
		PostProcess* postProcess = pProcessChain->getPostProcess(PostProcess::Type::ColorGrading);
		if (!postProcess)
			return;
		ColorGrading* colorGrading = static_cast<ColorGrading*>(postProcess);
		//if (colorGrading)
		colorGrading->setColorChart(tempColorChartPtr.get());
	}

	void TODManager::setCurrentCloudDiffuseByType(int type, const String& name)
	{
		EchoZoneScoped;

		if (type >= eCloudType_Max)
			return;

		if (mCurrentDiffuseName[type] == name)
			return;
		if (mSceneManager == nullptr)
			return;

		mCurrentDiffuseName[type] = name;

		Cloud* pCloud = mSceneManager->getCloud();
		if (!pCloud)
			return;

		TexturePtr texPtr;
		std::map<String, TexturePtr>::const_iterator it = mCloudSeaTextureMap.find(name);
		if (it != mCloudSeaTextureMap.end())
		{
			texPtr = it->second;
		}

		pCloud->setCloudTextureFromTOD((CloudType)type, texPtr);
	}
	void TODManager::setCurrentCloudDiffuseNameByCom(int type, const String& name)
	{
		EchoZoneScoped;

		if (type >= eCloudType_Max)
			return;

		if (mCurrentDiffuseName[type] == name)
			return;
		if (mSceneManager == nullptr)
			return;

		mCurrentDiffuseName[type] = name;
	}
}
namespace Echo {

	TODManager* TODManager::instance() {
		EchoZoneScoped;

		return TODSystem::instance()->GetMainManager();
	}

	TODManager::TODManager(const Name& name) : TODManager() {
		EchoZoneScoped;

		mName = name;
	}

	void TODManager::setSceneManager(SceneManager* mng) {
		EchoZoneScoped;

		mSceneManager = mng;
		if (mSceneManager) mSceneManager->setTODManager(this);
	}

	WeatherManager* TODManager::getWeatherManager() {
		EchoZoneScoped;

		if (mSceneManager && mSceneManager->getWeatherManager()) {
			return mSceneManager->getWeatherManager();
		}
		else {
			return WeatherSystem::instance()->GetManager(mName);
		}
	}
	EnvironmentLight* TODManager::getEnvironmentLight() {
		EchoZoneScoped;

		if (mSceneManager && mSceneManager->getEnvironmentLight()) {
			return mSceneManager->getEnvironmentLight();
		}
		else {
			return EnvironmentLight::instance();
		}
	}
	ProtocolComponentManager* TODManager::getProtocolComponentManager() {
		EchoZoneScoped;

		if (mSceneManager && mSceneManager->getProtocolComponentManager()) {
			return mSceneManager->getProtocolComponentManager();
		}
		else {
			return ProtocolComponentSystem::instance()->GetManager(mName);
		}
	}

	TODSystem::TODSystem() {
		EchoZoneScoped;


	}
	TODSystem::~TODSystem() {
		EchoZoneScoped;


	}

	void TODSystem::setLogSwitch(LogType type)
	{
		if (type >= LogType::LT_End) return;

		size_t offset = static_cast<size_t>(type);
		m_showLog |= static_cast<size_t>((size_t)1 << offset);
	}
	bool TODSystem::showLog(LogType type)
	{
		if (type >= LogType::LT_End) return false;

		size_t offset = static_cast<size_t>(type);
		return m_showLog & static_cast<size_t>((size_t)1 << offset);
	}
}
