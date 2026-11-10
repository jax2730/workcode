#ifndef __EchoRenderHelper_H__
#define __EchoRenderHelper_H__

#include "EchoPrerequisites.h"
#include "EchoMaterialWrapper.h"
#include "EchoMesh.h"
#include "EchoMovableObject.h"
#include "EchoRenderable.h"

namespace Echo
{
	class TrafficObject :public MovableObject
	{
	public:
		TrafficObject(const Name& name);
		~TrafficObject();

		class TrafficObjectSection;

		void setBounds(const Echo::AxisAlignedBox& aabb)
		{
			mBounds = aabb;
		}
		const Echo::AxisAlignedBox& getBounds() const
		{
			return mBounds;
		}

		void setUseDepthTest(bool use)
		{
			mCurrentSection->mUseDepthTest = use;
		}
		bool getUseDepthTest(void) const
		{
			return mCurrentSection->mUseDepthTest;
		}

		virtual void resetTempAreas();
		void clear();

		size_t getSectionSize();
		TrafficObjectSection* getSection(unsigned int index, EM_LOD_LEVEL lodlevel = LOD_LEVEL_0) const;
		void normal(const Vector3& norm);
		void position(const Vector3& pos);
		void tangent(const Vector4& tan);
		void colour(const ColorValue& col);
		void colour(float r, float g, float b, float a = 1.0f);
		void index(uint32 idx);
		void textureCoord(float u, float v);

		void resizeTempVertexBufferIfNeeded(size_t numVerts);
		void resizeTempIndexBufferIfNeeded(size_t numInds);
		void copyTempVertexToBuffer();

		void resizeVertexAndIndexCount(size_t numVerts, size_t numInds);

		void pre_begin(EM_LOD_LEVEL lodlevel = LOD_LEVEL_0);
		void pre_begin(size_t numVerts, size_t numInds, const String& VSFileName, const String& PSFileName,
			ECHO_DRAW opType = ECHO_TRIANGLE_LIST, EM_LOD_LEVEL lodlevel = LOD_LEVEL_0);

		virtual void begin(const String& VSFileName, const String& PSFileName,
			ECHO_DRAW opType = ECHO_TRIANGLE_LIST, EM_LOD_LEVEL lodlevel = LOD_LEVEL_0);
		virtual TrafficObjectSection * end(void);

		TrafficObjectSection * pre_end(void);
		void delay_end(void);

		void setVertexCount(size_t count) { mVertexCount = count; };
		void setIndexCount(size_t count) { mIndexCount = count; };
		//------------------------------------------------------------------
		// Override from MovableObject
		virtual Echo::MovableObjectType getMovableType() const override;
		virtual const Echo::AxisAlignedBox& getBoundingBox() const override;
		virtual float getBoundingRadius() const override;
		virtual void _notifyCurrentCamera(Camera* cam) override;
		virtual void updateRenderQueue(Echo::RenderQueue* pRenderQueue) override;
		virtual bool getCulled() override { return mCulled; };

		void	setRenderDistance(float distance) { mRenderDistance = distance; };
		void	setLodDistance(EM_LOD_LEVEL lodlevel, float distance) { m_fLodDistance[lodlevel] = distance; };
		//------------------------------------------------------------------

		class TrafficObjectSection :public Renderable, public EchoAllocatedObject
		{
			friend TrafficObject;
		public:
			TrafficObjectSection(TrafficObject* parent, const String& VSFileName, const String& PSFileName, ECHO_DRAW opType);
			~TrafficObjectSection();

			void clearData();
			void touchMaterial();
			void compressVertexData(int);
			void generateShaderMask();
			void setShaderMacro(const String& vsMacro, const String& psMacro);
			void setCullMode(ECHO_CULL_MODE mode);
			void setMaterial(const MaterialWrapper& material);
			void setIndexCount(uint32 size) { mIndexCount = size; }
			void setUseVertexColor(bool inUseVertexColor) { mUseVertexColor = inUseVertexColor; }
			void setUseDiffuseTexCoord(bool inUseDiffuseTexCoord) { mUseDiffuseTexCoord = inUseDiffuseTexCoord; }
			void setUseDepthTest(bool inUseDepthTest) { mUseDepthTest = inUseDepthTest; }
			void setUseDepthWrite(bool inUseDepthWrite) { mUseDepthWrite = inUseDepthWrite; }
			void setUsetMaterialDiffuse(bool inUseMaterialDiffuse) { mUseMaterialDiffuse = inUseMaterialDiffuse; }

			//------------------------------------------------------------------
			// Override from Renderable
			virtual void getWorldTransforms(Echo::DBMatrix4* xform) const override;
			virtual void getWorldTransforms(const Echo::DBMatrix4** xform) const override;
			virtual Echo::RenderOperation* getRenderOperation() override;
			virtual Echo::Material* getMaterial() const override;
			const MaterialWrapper& getMaterialWrapper() const override;
			//------------------------------------------------------------------

			TrafficObject* mParent;
			String mShaderVSFileName;
			String mShaderPSFileName;
			String mVSDX;
			String mPSDX;
			String mVSDXGenerateMask;
			String mPSDXGenerateMask;
			String mVSShaderMacro;
			String mPSShaderMacro;
			uint32 mVertexSize = 0;
			uint32 mVertexCount = 0;
			uint32 mIndexCount = 0;
			uint32 mExistedVertexCount = 0;
			uint32 mExistedIndexCount = 0;
			MaterialPtr mMaterial;
			RenderOperation mRenderOperation;
			VertexData*	m_pVertexData;
			IndexData* m_pIndexData;
			ECHO_CULL_MODE	mCullMode = ECHO_CULL_NONE;
			bool mUseDepthTest;
			bool mUseVertexColor;
			bool mUseDiffuseTexCoord;
			bool mUseMaterialDiffuse;
			bool m32BitIndices;
			bool mUseDepthWrite;
			bool mHadTouchMaterial;
			bool mDelMaterial = true;

			MemoryDataStreamPtr mTempIndexBuffer;
			MemoryDataStreamPtr mTempVertexBuffer;

			size_t mTempVertexSize;
			size_t mTempIndexSize;

			struct TempVertex
			{
				Vector3 position;
				Vector3 normal;
				Vector4 tangent;
				Vector4 texCoord[ECHO_MAX_TEXTURE_COORD_SETS];
				uint16 texCoordDims[ECHO_MAX_TEXTURE_COORD_SETS];
				ColorValue colour;
			};
			TempVertex mTempVertex;
		};

		typedef vector<TrafficObjectSection*>::type SectionList;

	protected:
		bool			mCulled = false;
		float			mRenderDistance = 1000.0f;

		EM_LOD_LEVEL	mCurrLodLevel = LOD_LEVEL_0;
		float           m_fLodDistance[LOD_LEVEL_NUM];

	protected:

		/*struct TempVertex
		{
			Vector3 position;
			Vector3 normal;
			Vector4 tangent;
			Vector4 texCoord[ECHO_MAX_TEXTURE_COORD_SETS];
			uint16 texCoordDims[ECHO_MAX_TEXTURE_COORD_SETS];
			ColorValue colour;
		};*/

		int	m_RenderPriority = 0;
		float mRadius;
		bool mAnyIndexed;
		bool mFirstVertex;
		bool mTempVertexPending;
		bool mCurrentUpdating;
		bool mDataCompress;
		size_t mDeclSize;
		//size_t mTempVertexSize;
		//size_t mTempIndexSize;
		size_t mVertexCount = 0;
		size_t mIndexCount = 0;
		uint16 mTexCoordIndex;

		//TempVertex mTempVertex;
		Echo::AxisAlignedBox mBounds;
		//MemoryDataStreamPtr mTempIndexBuffer;
		//MemoryDataStreamPtr mTempVertexBuffer;
		MaterialPtr  mCustomMaterial;

		std::vector<SectionList> mSectionList;
		TrafficObjectSection * mCurrentSection;

	public:
		bool delayEnd = false;//用于在多次调用begin()之后延迟调用end()
	};
}

#endif
