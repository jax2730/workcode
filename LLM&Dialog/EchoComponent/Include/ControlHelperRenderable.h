///////////////////////////////////////////////////////////////////////////////
//
// @file ControlHelperRenderable.h
// @author luoshuquan
// @date 2022/07/22 Friday
// @version 1.0
//
// @copyright Copyright (c) PixelSoft Corporation. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __ControlHelperRenderable_H__
#define __ControlHelperRenderable_H__

#include "EchoVector3.h"
#include "EchoColorValue.h"

namespace EchoBox
{
	// Some useful object to identify simple object like box, shpere, plane etc.
	//
	class ControlHelperRenderable
	{
	public:
		ControlHelperRenderable(Echo::SceneManager* sMgr);
		virtual ~ControlHelperRenderable();

		Echo::SceneNode* GetSceneNode();
		const Echo::DVector3& getWorldPosition();
		void setPosition(const Echo::Vector3& position);
		const Echo::Vector3& getPosition() const
		{
			return mPosition;
		}

		void setScale(float scale);
		float getScale() const
		{
			return mScale;
		}

		void setColor(const Echo::ColorValue& color);
		const Echo::ColorValue& getColor() const
		{
			return mColor;
		}

		void setHighlight(bool highlight);
		bool getHighlight() const
		{
			return mHighlight;
		}

		void setVisible(bool value);
		bool getVisible();

		virtual std::pair<bool, float> hitTest(const Echo::Ray& ray) const = 0;

	protected:
		Echo::Vector3 mPosition{ 0.f };
		float mScale = 1.f;
		Echo::ColorValue mColor{ Echo::ColorValue::White };
		bool mHighlight = false;
		class HelperRenderable* mRenderable = nullptr;
	};

	class BoxHelper : public ControlHelperRenderable
	{
	public:
		BoxHelper(Echo::SceneManager* sMgr);
		~BoxHelper();

		void setBox(const Echo::Vector3 box[8]);
		void setBox(float lenght, float width, float height,
			const Echo::Vector3& center = Echo::Vector3::ZERO);

		virtual std::pair<bool, float> hitTest(const Echo::Ray& ray) const override;

	private:
		Echo::Vector3 mVertices[8];
	};

	class UnitBoxHelper {
	};

	class SphereHelper : public ControlHelperRenderable
	{
	public:
		SphereHelper(Echo::SceneManager* sMgr);
		~SphereHelper();

		bool isInitData();
		void setRadius(float radius);
		float getRadius() const
		{
			return mRadius;
		}

		std::pair<bool, float> hitTest(const Echo::Ray& ray) const override;

	private:
		float mRadius = 0.5f;
	};

	class ArrowHelper : public ControlHelperRenderable
	{
	public:
		ArrowHelper(Echo::SceneManager* sMgr);
		~ArrowHelper();

		bool isInitData();
		void setRadius(float radius);
		
		std::pair<bool, float> hitTest(const Echo::Ray& ray) const override;
		float getRadius() const
		{
			return mRadius;
		}
		void SetNormal(const Echo::Vector3& v);
	private:
		float mRadius = 0.5f;
		Echo::Vector3 mNormal = Echo::Vector3::UNIT_Y;

		Echo::Vector3 mVertices[26];
		unsigned short mIndexArray[132];
	};
}

#endif // __ControlHelperRenderable_H__
