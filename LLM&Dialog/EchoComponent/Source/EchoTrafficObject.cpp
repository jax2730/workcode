#include "EchoComponentStableHeaders.h"
#include "EchoTrafficObject.h"
#include "EchoShaderContentManager.h"
#include "EchoSubMesh.h"
#include "EchoMaterialManager.h"
#include "EchoDynamicVideoParamManager.h"


namespace Echo {
	const int TempInitialSize = 50;
	const int TempVertexsizeGuess = sizeof(float) * 12;
	const int TempInitialVertexSize = TempVertexsizeGuess * TempInitialSize;
	const int TempInitialIndexSize = sizeof(uint32) * TempInitialSize;

	TrafficObject::TrafficObject(const Name& name)
		: MovableObject(name)
		, mRadius(0)
		, mAnyIndexed(false)
		, mFirstVertex(true)
		, mTempVertexPending(false)
		, mCurrentUpdating(false)
		, mDataCompress(false)
		, mDeclSize(0)
		/*, mTempVertexSize(TempInitialVertexSize)
		, mTempIndexSize(TempInitialIndexSize)*/
		, mTexCoordIndex(0)
		, mCurrentSection(0)
	{
		for (int i = 0; i < LOD_LEVEL_NUM; ++i)
		{
			float d = i * 100.f;
			m_fLodDistance[i] = d;
		}
		mSectionList.resize(LOD_LEVEL_NUM);
	}

	TrafficObject::~TrafficObject()
	{
		clear();
	}

	void TrafficObject::clear(void)
	{
		DEF_THREAD_CHEEK(DEF_THREAD_TYPE_ALL);
		resetTempAreas();
		for(auto& renderVec : mSectionList)
		{
			for (auto& render : renderVec)
			{
				delete render;
			}
			renderVec.clear();
		}
		//mSectionList.clear();
		mRadius = 0;
		mBounds.setNull();
		mAnyIndexed = false;
	}

	void TrafficObject::resetTempAreas(void)
	{
		if(mCurrentSection)
		{
			mCurrentSection->mTempIndexBuffer.setNull();
			mCurrentSection->mTempVertexBuffer.setNull();
			mCurrentSection->mTempVertexSize = TempInitialVertexSize;
			mCurrentSection->mTempIndexSize = TempInitialIndexSize;
		}
	}

	size_t TrafficObject::getSectionSize()
	{
		return mSectionList.size();
	}

	TrafficObject::TrafficObjectSection*
		TrafficObject::getSection(unsigned int inIndex, EM_LOD_LEVEL lodlevel) const
	{
		DEF_THREAD_CHEEK(DEF_THREAD_TYPE_ALL);
		if (lodlevel >= mSectionList.size() || inIndex >= mSectionList[lodlevel].size())
			ECHO_EXCEPT(Exception::ERR_INVALIDPARAMS,
			"Index out of bounds.",
			"TrafficObject::getSection");
		return mSectionList[lodlevel][inIndex];
	}

	void TrafficObject::position(const Vector3& pos)
	{
		DEF_THREAD_CHEEK(DEF_THREAD_TYPE_ALL)
			if (!mCurrentSection)
			{
				ECHO_EXCEPT(Exception::ERR_INVALIDPARAMS,
					"You must call begin() before this method",
					"TrafficObject::position");
			}
		if (mTempVertexPending)
		{
			// bake current vertex
			//Material* temp = mCurrentSection->getMaterial();
			//temp->m_desc.IADesc.StreamArray[0].nStride = mDeclSize;
			if (mDeclSize != 0)
			{
				resizeTempVertexBufferIfNeeded(mVertexCount);
				resizeTempIndexBufferIfNeeded(mIndexCount);
			}
			copyTempVertexToBuffer();
			mFirstVertex = false;
		}

		if (mFirstVertex && !mCurrentUpdating)
		{
			//size_t elementIndex = mCurrentSection->getMaterial()->m_desc.IADesc.nElementCount;
			Material* temp = mCurrentSection->getMaterial();

			//if (mDataCompress)
			if(temp->m_desc.IADesc.nElementCount)
			for (size_t e = 0; e < temp->m_desc.IADesc.nElementCount; ++e)
			{
				const ECHO_IA_ELEMENT& iaElem = temp->m_desc.IADesc.ElementArray[e];
				if (iaElem.Semantics == ECHO_POSITION0)
				{
					if (iaElem.eType == ECHO_FLOAT3)
					{
						Material::addElement(temp->m_desc, ECHO_POSITION0, ECHO_FLOAT3, 0, 0);
						mDeclSize += sizeof(float) * 3;
					}
					else
					{
						Material::addElement(temp->m_desc, ECHO_POSITION0, ECHO_FP16_VEC4, 0, 0);
						mDeclSize += VertexType::typeSize(ECHO_FP16_VEC4);
					}
					break;
				}
			}
			else
			{
				if (mDataCompress)
				{
					Material::addElement(temp->m_desc, ECHO_POSITION0, ECHO_FP16_VEC4, 0, 0);
					mDeclSize += VertexType::typeSize(ECHO_FP16_VEC4);
				}
				else
				{
					Material::addElement(temp->m_desc, ECHO_POSITION0, ECHO_FLOAT3, 0, 0);
					mDeclSize += sizeof(float) * 3;
				}
			}

			/*if (mDataCompress)
			{
				Material::addElement(temp->m_desc, ECHO_POSITION0, ECHO_FP16_VEC4, 0, 0);
				mDeclSize += VertexType::typeSize(ECHO_FP16_VEC4);
			}
			else
			{
				Material::addElement(temp->m_desc, ECHO_POSITION0, ECHO_FLOAT3, 0, 0);
				mDeclSize += sizeof(float) * 3;
			}*/

			//temp->m_desc.IADesc.ElementArray[elementIndex].nStream = 0;
			//temp->m_desc.IADesc.ElementArray[elementIndex].nOffset = (uint32)mDeclSize;
			//temp->m_desc.IADesc.ElementArray[elementIndex].eType = ECHO_FLOAT3;
			//temp->m_desc.IADesc.ElementArray[elementIndex].Semantics = ECHO_POSITION0;
			//mCurrentSection->getMaterial()->m_desc.IADesc.nElementCount = (uint32)elementIndex + 1;

		}

		mCurrentSection->mTempVertex.position.x = pos.x;
		mCurrentSection->mTempVertex.position.y = pos.y;
		mCurrentSection->mTempVertex.position.z = pos.z;

		//mCurrentSection->addVertexCount();
		// update bounds
		mBounds.merge(pos);
		mRadius = std::max(mRadius, pos.length());

		// reset current texture coord
		mTexCoordIndex = 0;

		mTempVertexPending = true;
	}

	void TrafficObject::normal(const Vector3& norm)
	{
		DEF_THREAD_CHEEK(DEF_THREAD_TYPE_ALL)
			if (!mCurrentSection)
			{
				ECHO_EXCEPT(Exception::ERR_INVALIDPARAMS,
					"You must call begin() before this method",
					"TrafficObject::normal");
			}
		if (mFirstVertex && !mCurrentUpdating)
		{
			// defining declaration
			//size_t elementIndex = mCurrentSection->getMaterial()->m_desc.IADesc.nElementCount;
			Material* temp = mCurrentSection->getMaterial();
			//temp->m_desc.IADesc.ElementArray[elementIndex].nStream = 0;
			//temp->m_desc.IADesc.ElementArray[elementIndex].nOffset = (uint32)mDeclSize;
			//temp->m_desc.IADesc.ElementArray[elementIndex].eType = ECHO_FLOAT3;
			//temp->m_desc.IADesc.ElementArray[elementIndex].Semantics = ECHO_NORMAL0;
			//mCurrentSection->getMaterial()->m_desc.IADesc.nElementCount = (uint32)elementIndex + 1;

			for (size_t e = 0; e < temp->m_desc.IADesc.nElementCount; ++e)
			{
				const ECHO_IA_ELEMENT& iaElem = temp->m_desc.IADesc.ElementArray[e];
				if (iaElem.Semantics == ECHO_NORMAL0)
				{
					if (iaElem.eType == ECHO_FLOAT3)
					{
						Material::addElement(temp->m_desc, ECHO_NORMAL0, ECHO_FLOAT3, 0, 0);
						mDeclSize += sizeof(float) * 3;
					}
					else
					{
						Material::addElement(temp->m_desc, ECHO_NORMAL0, ECHO_FP16_VEC4, 0, 0);
						mDeclSize += VertexType::typeSize(ECHO_FP16_VEC4);
					}
					break;
				}
			}

			/*if (mDataCompress)
			{
				Material::addElement(temp->m_desc, ECHO_NORMAL0, ECHO_FP16_VEC4, 0, 0);
				mDeclSize += VertexType::typeSize(ECHO_FP16_VEC4);
			}
			else
			{
				Material::addElement(temp->m_desc, ECHO_NORMAL0, ECHO_FLOAT3, 0, 0);
				mDeclSize += sizeof(float) * 3;
			}*/
		}
		mCurrentSection->mTempVertex.normal.x = norm.x;
		mCurrentSection->mTempVertex.normal.y = norm.y;
		mCurrentSection->mTempVertex.normal.z = norm.z;
	}

	void TrafficObject::tangent(const Vector4& tan)
	{
		DEF_THREAD_CHEEK(DEF_THREAD_TYPE_ALL)
			if (!mCurrentSection)
			{
				ECHO_EXCEPT(Exception::ERR_INVALIDPARAMS,
					"You must call begin() before this method",
					"TrafficObject::tangent");
			}
		if (mFirstVertex && !mCurrentUpdating)
		{
			// defining declaration
			//size_t elementIndex = mCurrentSection->getMaterial()->m_desc.IADesc.nElementCount;
			Material* temp = mCurrentSection->getMaterial();
			//temp->m_desc.IADesc.ElementArray[elementIndex].nStream = 0;
			//temp->m_desc.IADesc.ElementArray[elementIndex].nOffset = (uint32)mDeclSize;
			//temp->m_desc.IADesc.ElementArray[elementIndex].eType = ECHO_FLOAT4;
			//temp->m_desc.IADesc.ElementArray[elementIndex].Semantics = ECHO_TANGENT0;
			//mCurrentSection->getMaterial()->m_desc.IADesc.nElementCount = (uint32)elementIndex + 1;

			for (size_t e = 0; e < temp->m_desc.IADesc.nElementCount; ++e)
			{
				const ECHO_IA_ELEMENT& iaElem = temp->m_desc.IADesc.ElementArray[e];
				if (iaElem.Semantics == ECHO_TANGENT0)
				{
					if (iaElem.eType == ECHO_FLOAT4)
					{
						Material::addElement(temp->m_desc, ECHO_TANGENT0, ECHO_FLOAT4, 0, 0);
						mDeclSize += sizeof(float) * 4;
					}
					else
					{
						Material::addElement(temp->m_desc, ECHO_TANGENT0, ECHO_FP16_VEC4, 0, 0);
						mDeclSize += VertexType::typeSize(ECHO_FP16_VEC4);  // sizeof(float) * 4;
					}
					break;
				}
			}

			//if (mDataCompress)
			//{
			//	Material::addElement(temp->m_desc, ECHO_TANGENT0, ECHO_FP16_VEC4, 0, 0);
			//	mDeclSize += VertexType::typeSize(ECHO_FP16_VEC4);  // sizeof(float) * 4;
			//}
			//else
			//{
			//	Material::addElement(temp->m_desc, ECHO_TANGENT0, ECHO_FLOAT4, 0, 0);
			//	mDeclSize += sizeof(float) * 4;
			//}
		}
		mCurrentSection->mTempVertex.tangent.x = tan.x;
		mCurrentSection->mTempVertex.tangent.y = tan.y;
		mCurrentSection->mTempVertex.tangent.z = tan.z;
		mCurrentSection->mTempVertex.tangent.w = tan.w;
	}

	void TrafficObject::colour(const ColorValue& col)
	{
		DEF_THREAD_CHEEK(DEF_THREAD_TYPE_ALL)
		colour(col.r, col.g, col.b, col.a);
	}

	void TrafficObject::colour(float r, float g, float b, float a)
	{
		DEF_THREAD_CHEEK(DEF_THREAD_TYPE_ALL)
			if (!mCurrentSection)
			{
				ECHO_EXCEPT(Exception::ERR_INVALIDPARAMS,
					"You must call begin() before this method",
					"ManualObject::colour");
			}
		if (mFirstVertex && !mCurrentUpdating)
		{
			// defining declaration 
			//size_t elementIndex = mCurrentSection->getMaterial()->m_desc.IADesc.nElementCount;
			Material* temp = mCurrentSection->getMaterial();
			//temp->m_desc.IADesc.ElementArray[elementIndex].nStream = 0;
			//temp->m_desc.IADesc.ElementArray[elementIndex].nOffset = (uint32)mDeclSize;
			//temp->m_desc.IADesc.ElementArray[elementIndex].eType = ECHO_FLOAT4;
			//temp->m_desc.IADesc.ElementArray[elementIndex].Semantics = ECHO_COLOR0;
			//mCurrentSection->getMaterial()->m_desc.IADesc.nElementCount = (uint32)elementIndex + 1;

			if (mDataCompress)
			{
				Material::addElement(temp->m_desc, ECHO_COLOR0, ECHO_UBYTE4N, 0, mTexCoordIndex);
				mDeclSize += VertexType::typeSize(ECHO_UBYTE4N);
			}
			else
			{
				Material::addElement(temp->m_desc, ECHO_COLOR0, ECHO_FLOAT4, 0, mTexCoordIndex);
				mDeclSize += sizeof(float) * 4;
			}


			mCurrentSection->setUseVertexColor(true);
		}
		mCurrentSection->mTempVertex.colour.r = r;
		mCurrentSection->mTempVertex.colour.g = g;
		mCurrentSection->mTempVertex.colour.b = b;
		mCurrentSection->mTempVertex.colour.a = a;

	}

	void TrafficObject::textureCoord(float u, float v)
	{
		DEF_THREAD_CHEEK(DEF_THREAD_TYPE_ALL)
			if (!mCurrentSection)
			{
				ECHO_EXCEPT(Exception::ERR_INVALIDPARAMS,
					"You must call begin() before this method",
					"TrafficObject::textureCoord");
			}
		if (mFirstVertex && !mCurrentUpdating)
		{
			// defining declaration
			//size_t elementIndex = mCurrentSection->getMaterial()->m_desc.IADesc.nElementCount;
			Material* temp = mCurrentSection->getMaterial();
			//temp->m_desc.IADesc.ElementArray[elementIndex].nStream = 0;
			//temp->m_desc.IADesc.ElementArray[elementIndex].nOffset = (uint32)mDeclSize;
			//temp->m_desc.IADesc.ElementArray[elementIndex].eType = ECHO_FLOAT2;
			//temp->m_desc.IADesc.ElementArray[elementIndex].Semantics = (ECHO_SEMANTICS)(ECHO_TEXCOORD0 + mTexCoordIndex);
			//         temp->m_desc.IADesc.ElementArray[elementIndex].nIndex = mTexCoordIndex;
			//mCurrentSection->getMaterial()->m_desc.IADesc.nElementCount = (uint32)elementIndex + 1;

			for (size_t e = 0; e < temp->m_desc.IADesc.nElementCount; ++e)
			{
				const ECHO_IA_ELEMENT& iaElem = temp->m_desc.IADesc.ElementArray[e];
				if (iaElem.Semantics == (ECHO_SEMANTICS)(ECHO_TEXCOORD0 + mTexCoordIndex))
				{
					if (iaElem.eType == ECHO_FLOAT2)
					{
						Material::addElement(temp->m_desc, (ECHO_SEMANTICS)(ECHO_TEXCOORD0 + mTexCoordIndex), ECHO_FLOAT2, 0, mTexCoordIndex);

						mDeclSize += sizeof(float) * 2;
					}
					else
					{
						Material::addElement(temp->m_desc, (ECHO_SEMANTICS)(ECHO_TEXCOORD0 + mTexCoordIndex), ECHO_FP16_VEC2, 0, mTexCoordIndex);

						mDeclSize += VertexType::typeSize(ECHO_FP16_VEC2);// sizeof(float) * 2;
					}
					break;
				}
			}

			//if (mDataCompress)
			//{
			//	Material::addElement(temp->m_desc, (ECHO_SEMANTICS)(ECHO_TEXCOORD0 + mTexCoordIndex), ECHO_FP16_VEC2, 0, mTexCoordIndex);

			//	mDeclSize += VertexType::typeSize(ECHO_FP16_VEC2);// sizeof(float) * 2;
			//}
			//else
			//{
			//	Material::addElement(temp->m_desc, (ECHO_SEMANTICS)(ECHO_TEXCOORD0 + mTexCoordIndex), ECHO_FLOAT2, 0, mTexCoordIndex);

			//	mDeclSize += sizeof(float) * 2;
			//}

			mCurrentSection->mUseDiffuseTexCoord = true;
		}
		mCurrentSection->mTempVertex.texCoordDims[mTexCoordIndex] = 2;
		mCurrentSection->mTempVertex.texCoord[mTexCoordIndex].x = u;
		mCurrentSection->mTempVertex.texCoord[mTexCoordIndex].y = v;

		++mTexCoordIndex;
	}

	void TrafficObject::index(uint32 idx)
	{
		DEF_THREAD_CHEEK(DEF_THREAD_TYPE_ALL)
			if (!mCurrentSection)
			{
				ECHO_EXCEPT(Exception::ERR_INVALIDPARAMS,
					"You must call begin() before this method",
					"TrafficObject::index");
			}
		mAnyIndexed = true;
		if (idx >= 65536)
			mCurrentSection->m32BitIndices = true;

		// make sure we have index data
		RenderOperation* rop = mCurrentSection->getRenderOperation();
		rop->useIndexes = true;
		uint32 indexCount = mCurrentSection->mIndexCount;
		resizeTempIndexBufferIfNeeded(++indexCount);
		uint16* pIndexBuff = (uint16*)mCurrentSection->mTempIndexBuffer->getPtr();
		pIndexBuff[indexCount - 1] = idx;
		mCurrentSection->mIndexCount = indexCount;
	}

	void TrafficObject::resizeTempVertexBufferIfNeeded(size_t numVerts)
	{
		// Calculate byte size
		// Use decl if we know it by now, otherwise default size to pos/norm/texcoord*2
		size_t newSize;
		if (!mFirstVertex)
		{
			newSize = mDeclSize * numVerts;
		}
		else
		{
			// estimate - size checks will deal for subsequent verts
			newSize = TempVertexsizeGuess * numVerts;
		}
		if (newSize > mCurrentSection->mTempVertexSize || mCurrentSection->mTempVertexBuffer.isNull())
		{
			if (mCurrentSection->mTempVertexBuffer.isNull())
			{
				// init
				newSize = mCurrentSection->mTempVertexSize;
			}
			else
			{
				// increase to at least double current
				newSize = std::max(newSize, mCurrentSection->mTempVertexSize * 2);
			}
			// copy old data

			MemoryDataStreamPtr vtxDataStream(new MemoryDataStream(newSize));
			if (!mCurrentSection->mTempVertexBuffer.isNull())
			{
				memcpy(vtxDataStream->getPtr(), mCurrentSection->mTempVertexBuffer->getPtr(), mCurrentSection->mTempVertexSize);
			}
			mCurrentSection->mTempVertexBuffer = vtxDataStream;


			//char* tmp = mTempVertexBuffer;
			//mTempVertexBuffer = (char*)malloc(sizeof(char) * newSize);
			//if (tmp)
			//{
			//	memcpy(mTempVertexBuffer, tmp, mTempVertexSize);
			//	// delete old buffer
			//	free(tmp);
			//}
			mCurrentSection->mTempVertexSize = newSize;
		}
	}

	void TrafficObject::resizeTempIndexBufferIfNeeded(size_t numInds)
	{
		size_t newSize = numInds * sizeof(uint16);
		if (newSize > mCurrentSection->mTempIndexSize || mCurrentSection->mTempIndexBuffer.isNull())
		{
			if (mCurrentSection->mTempIndexBuffer.isNull())
			{
				// init
				newSize = mCurrentSection->mTempIndexSize;
			}
			else
			{
				// increase to at least double current
				newSize = std::max(newSize, mCurrentSection->mTempIndexSize * 2);
			}


			MemoryDataStreamPtr vtxDataStream(new MemoryDataStream(newSize));
			if (!mCurrentSection->mTempIndexBuffer.isNull())
			{
				memcpy(vtxDataStream->getPtr(), mCurrentSection->mTempIndexBuffer->getPtr(), mCurrentSection->mTempIndexSize);
			}
			mCurrentSection->mTempIndexBuffer = vtxDataStream;


			//numInds = newSize / sizeof(uint16);
			//uint16* tmp = mTempIndexBuffer;
			//mTempIndexBuffer = (uint16*)malloc(sizeof(uint16) * numInds);
			//if (tmp)
			//{
			//	memcpy(mTempIndexBuffer, tmp, mTempIndexSize);
			//	free(tmp);
			//}
			mCurrentSection->mTempIndexSize = newSize;
		}
	}

	void TrafficObject::copyTempVertexToBuffer(void)
	{
		DEF_THREAD_CHEEK(DEF_THREAD_TYPE_ALL);
		mTempVertexPending = false;
		mCurrentSection->mVertexCount++;
		resizeTempVertexBufferIfNeeded(mCurrentSection->mVertexCount);
		mCurrentSection->mVertexSize = (uint32)mDeclSize;

		// get base pointer
		char* pBase = (char*)mCurrentSection->mTempVertexBuffer->getPtr() + (mDeclSize * (mCurrentSection->mVertexCount - 1));
		Material* material = mCurrentSection->getMaterial();
		size_t elementCount = material->m_desc.IADesc.nElementCount;
		for (size_t i = 0; i < elementCount; ++i)
		{
			float* pFloat = 0;
			uint8*  pStart = 0;
			const ECHO_IA_ELEMENT& element = material->m_desc.IADesc.ElementArray[i];
			switch (element.eType)
			{
			case ECHO_FLOAT1:
			case ECHO_FLOAT2:
			case ECHO_FLOAT3:
			case ECHO_FLOAT4:
				pFloat = static_cast<float*>(static_cast<void*>((unsigned char*)(pBase)+element.nOffset));
				break;
			default:
				pStart = (unsigned char*)(pBase)+element.nOffset;
				// nop ?
				break;
			};
			if (!pFloat && !pStart)
				continue;
			unsigned short dims;
			switch (element.Semantics)
			{
			case ECHO_POSITION0:
			case ECHO_POSITION1:
			case ECHO_POSITION2:
			case ECHO_POSITION3:
				if (pStart)
				{
					unsigned short *pUShort = (unsigned short *)pStart;
					*pUShort++ = Math::f32_to_u16(mCurrentSection->mTempVertex.position.x);
					*pUShort++ = Math::f32_to_u16(mCurrentSection->mTempVertex.position.y);
					*pUShort++ = Math::f32_to_u16(mCurrentSection->mTempVertex.position.z);
				}
				else
				{
					*pFloat++ = mCurrentSection->mTempVertex.position.x;
					*pFloat++ = mCurrentSection->mTempVertex.position.y;
					*pFloat++ = mCurrentSection->mTempVertex.position.z;
				}

				break;
			case ECHO_NORMAL0:
			case ECHO_NORMAL1:
			case ECHO_NORMAL2:
			case ECHO_NORMAL3:

				if (pStart)
				{
					unsigned short *pUShort = (unsigned short *)pStart;
					*pUShort++ = Math::f32_to_u16(mCurrentSection->mTempVertex.normal.x);
					*pUShort++ = Math::f32_to_u16(mCurrentSection->mTempVertex.normal.y);
					*pUShort++ = Math::f32_to_u16(mCurrentSection->mTempVertex.normal.z);
				}
				else
				{
					*pFloat++ = mCurrentSection->mTempVertex.normal.x;
					*pFloat++ = mCurrentSection->mTempVertex.normal.y;
					*pFloat++ = mCurrentSection->mTempVertex.normal.z;
				}


				break;
			case ECHO_TANGENT0:
			case ECHO_TANGENT1:
			case ECHO_TANGENT2:
			case ECHO_TANGENT3:

				if (pStart)
				{
					unsigned short *pUShort = (unsigned short *)pStart;
					*pUShort++ = Math::f32_to_u16(mCurrentSection->mTempVertex.tangent.x);
					*pUShort++ = Math::f32_to_u16(mCurrentSection->mTempVertex.tangent.y);
					*pUShort++ = Math::f32_to_u16(mCurrentSection->mTempVertex.tangent.z);
					*pUShort++ = Math::f32_to_u16(mCurrentSection->mTempVertex.tangent.w);
				}
				else
				{
					*pFloat++ = mCurrentSection->mTempVertex.tangent.x;
					*pFloat++ = mCurrentSection->mTempVertex.tangent.y;
					*pFloat++ = mCurrentSection->mTempVertex.tangent.z;
					*pFloat++ = mCurrentSection->mTempVertex.tangent.w;
				}


				break;
			case ECHO_TEXCOORD0:
			case ECHO_TEXCOORD1:
			case ECHO_TEXCOORD2:
			case ECHO_TEXCOORD3:
			case ECHO_TEXCOORD4:
			case ECHO_TEXCOORD5:
			case ECHO_TEXCOORD6:
			case ECHO_TEXCOORD7:

				if (pStart)
				{
					unsigned short *pUShort = (unsigned short *)pStart;
					if (element.eType == ECHO_FP16_VEC2)
					{
						dims = 2;
						for (uint16 t = 0; t < dims; ++t)
							*pUShort++ = Math::f32_to_u16(mCurrentSection->mTempVertex.texCoord[element.nIndex][t]);
						break;
					}

					if (element.eType == ECHO_FP16_VEC4)
					{
						dims = 4;
						for (uint16 t = 0; t < dims; ++t)
							*pUShort++ = Math::f32_to_u16(mCurrentSection->mTempVertex.texCoord[element.nIndex][t]);
						break;
					}
				}
				else
				{
					if (element.eType == ECHO_FLOAT1)
					{
						dims = 1;
						for (uint16 t = 0; t < dims; ++t)
							*pFloat++ = mCurrentSection->mTempVertex.texCoord[element.nIndex][t];
						break;
					}

					if (element.eType == ECHO_FLOAT2)
					{
						dims = 2;
						for (uint16 t = 0; t < dims; ++t)
							*pFloat++ = mCurrentSection->mTempVertex.texCoord[element.nIndex][t];
						break;
					}

					if (element.eType == ECHO_FLOAT3)
					{
						dims = 3;
						for (uint16 t = 0; t < dims; ++t)
							*pFloat++ = mCurrentSection->mTempVertex.texCoord[element.nIndex][t];
						break;
					}

					if (element.eType == ECHO_FLOAT4)
					{
						dims = 4;
						for (uint16 t = 0; t < dims; ++t)
							*pFloat++ = mCurrentSection->mTempVertex.texCoord[element.nIndex][t];
						break;
					}
				}



			case ECHO_COLOR0:
			case ECHO_COLOR1:
			case ECHO_COLOR2:
			case ECHO_COLOR3:

				if (pStart)
				{
					unsigned short *pUShort = (unsigned short *)pStart;
					if (element.eType == ECHO_FP16_VEC4)
					{
						dims = 4;
						for (uint16 t = 0; t < dims; ++t)
							*pUShort++ = Math::f32_to_u16(mCurrentSection->mTempVertex.colour[t]);
						break;
					}
				}
				else
				{
					if (element.eType == ECHO_FLOAT4)
					{
						dims = 4;
						for (size_t t = 0; t < dims; ++t)
							*pFloat++ = mCurrentSection->mTempVertex.colour[t];
						break;
					}
				}


			default:
				// nop ?
				break;
			};

		}
		if (mFirstVertex)
		{
			mCurrentSection->generateShaderMask();
		}
	}

	void TrafficObject::resizeVertexAndIndexCount(size_t numVerts, size_t numInds)
	{
		if (mTempVertexPending)
			mCurrentSection->mVertexCount = uint32(numVerts - 1);
		else
			mCurrentSection->mVertexCount = uint32(numVerts);
		mCurrentSection->mIndexCount = uint32(numInds);
	}

	void TrafficObject::pre_begin(EM_LOD_LEVEL lodlevel)
	{
		DEF_THREAD_CHEEK(DEF_THREAD_TYPE_ALL)
			if (mCurrentSection && !delayEnd)
			{
				ECHO_EXCEPT(Exception::ERR_INVALIDPARAMS,
					"You cannot call begin() again until after you call end()",
					"ManualObject::begin");
			}

		if (mSectionList.size() != 0)
		{
			mCurrentSection = mSectionList[lodlevel].back();
			mCurrentSection->mVertexCount = mCurrentSection->mExistedVertexCount;
			mCurrentSection->mIndexCount = mCurrentSection->mExistedIndexCount;
		}
	}

	void TrafficObject::pre_begin(size_t numVerts, size_t numInds, 
		const String& VSFileName, const String& PSFileName,ECHO_DRAW opType, EM_LOD_LEVEL lodlevel)
	{
		
		DEF_THREAD_CHEEK(DEF_THREAD_TYPE_ALL)
			if (mCurrentSection && !delayEnd)
			{
				ECHO_EXCEPT(Exception::ERR_INVALIDPARAMS,
					"You cannot call begin() again until after you call end()",
					"ManualObject::begin");
			}

		if (mSectionList.size() == 0 || 
			((mSectionList[lodlevel][0]->mExistedVertexCount == mVertexCount) &&
				(mSectionList[lodlevel][0]->mExistedIndexCount == mIndexCount))
		   )
		{
			begin(VSFileName, PSFileName, opType);

			mVertexCount = numVerts;
			mIndexCount = numInds;
		}
		else
		{
			pre_begin();
		}
	}

	void TrafficObject::begin(const String& VSFileName, const String& PSFileName,
		ECHO_DRAW opType, EM_LOD_LEVEL lodlevel)
	{
		DEF_THREAD_CHEEK(DEF_THREAD_TYPE_ALL)
			if (mCurrentSection && !delayEnd)
			{
				ECHO_EXCEPT(Exception::ERR_INVALIDPARAMS,
					"You cannot call begin() again until after you call end()",
					"ManualObject::begin");
			}

		mCurrentSection = new TrafficObjectSection(this, VSFileName, PSFileName, opType);
		mCurrentUpdating = false;
		mSectionList[lodlevel].push_back(mCurrentSection);

		RenderOperation* rop = mCurrentSection->getRenderOperation();
		rop->ResetVertexIndexCount();
		rop->useIndexes = false;

		Material* materialTemp = mCurrentSection->getMaterial();

		materialTemp->m_desc.IADesc.nElementCount = 0;
		materialTemp->m_desc.IADesc.nStreamCount = 1;
		materialTemp->m_desc.IADesc.StreamArray[0].bPreInstance = ECHO_FALSE;
		materialTemp->m_desc.IADesc.StreamArray[0].nRate = 1;
		materialTemp->m_desc.IADesc.StreamArray[0].nStride = 0;
		//materialTemp->m_desc.RenderState.CullMode = ECHO_CULL_BACK;

		mFirstVertex = true;
		mTempVertexPending = false;
		mDeclSize = 0;
		mTexCoordIndex = 0;
		mCurrentSection->mVertexCount = 0;
		mCurrentSection->mIndexCount = 0;
		mCurrentSection->mVertexSize = 0;
	}

	TrafficObject::TrafficObjectSection* TrafficObject::end(void)
	{
		DEF_THREAD_CHEEK(DEF_THREAD_TYPE_ALL)
			if (!mCurrentSection)
			{
				ECHO_EXCEPT(Exception::ERR_INVALIDPARAMS,
					"You cannot call end() until after you call begin()",
					"ManualObject::end");
			}
		if (mTempVertexPending)
		{
			// bake current vertex
			copyTempVertexToBuffer();
		}

		// pointer that will be returned
		TrafficObjectSection* result = NULL;

		RenderOperation* rop = mCurrentSection->getRenderOperation();
		// Check for empty content
		if (mCurrentSection->mVertexCount == 0 ||
			(rop->useIndexes && mCurrentSection->mIndexCount == 0))
		{
			// You're wasting my time sonny
			if (mCurrentUpdating)
			{
				// Can't just undo / remove since may be in the middle
				// Just allow counts to be 0, will not be issued to renderer

				// return the finished section (though it has zero vertices)
				result = mCurrentSection;
			}
			else
			{
				// First creation, can really undo
				// Has already been added to section list end, so remove
				for (SectionList& sections : mSectionList)
				{
					SectionList::iterator iter = std::find(sections.begin(), sections.end(), mCurrentSection);
					if (iter != sections.end())
					{
						//sections.erase(iter);
						delete mCurrentSection;
						*iter = nullptr;
						break;
					}
				}
			}
		}
		else // not an empty section
		{
			RenderSystem* renderSystem = Root::instance()->getRenderSystem();

			// Check buffer sizes
			//bool vbufNeedsCreating = true;
			//bool ibufNeedsCreating = rop->useIndexes;
			// Work out if we require 16 or 32-bit index buffers
			//bool use32BitIndices = mCurrentSection->get32BitIndices();

			mCurrentSection->clearData();
			//save cpu data for convertToMesh
			VertexData* pVertexCpuData = mCurrentSection->m_pVertexData;
			//pVertexCpuData->clearCpuStreamData();// ClearAllVertexCpuData();
			pVertexCpuData->m_vertexCount = mCurrentSection->mVertexCount;

			//uint32 vertexSize = (uint32)mDeclSize * pVertexCpuData->m_vertexCount;
			//MemoryDataStreamPtr vtxDataStream(new MemoryDataStream(vertexSize));
			//memcpy(vtxDataStream->getPtr(), mTempVertexBuffer, vertexSize);
			pVertexCpuData->addCpuStreamData(mCurrentSection->mTempVertexBuffer->getPtr(), mCurrentSection->mTempVertexBuffer, 0, uint16(mDeclSize));

			if (mDataCompress)
			{
				mCurrentSection->compressVertexData(1);
			}
			uint8* vertexPtr = mCurrentSection->m_pVertexData->GetVertexCpuData(0)->mp_start_memory;
			uint16 newVertexSize = mCurrentSection->m_pVertexData->GetVertexCpuData(0)->m_vertex_size;
			if (rop->useIndexes)
			{
				IndexData* pIndexCpuData = mCurrentSection->m_pIndexData;
				pIndexCpuData->m_indexCount = mCurrentSection->mIndexCount;

				//uint16 indexDataSize = (uint16)pIndexCpuData->m_indexCount * sizeof(uint16);

				//MemoryDataStreamPtr indexDataStream(new MemoryDataStream(indexDataSize));
				//memcpy(indexDataStream->getPtr(), mTempIndexBuffer, indexDataSize);
				pIndexCpuData->setCpuStreamData(mCurrentSection->mTempIndexBuffer->getPtr(), mCurrentSection->mTempIndexBuffer);
			}

			//
			rop->GetMeshData().Release();
			rop->GetMeshData().m_vertexCount = mCurrentSection->mVertexCount;
			rop->GetMeshData().m_vertexBuffer.push_back(renderSystem->createStaticVB(vertexPtr, (uint32)newVertexSize * rop->GetMeshData().m_vertexCount));

			if (rop->useIndexes)
			{
				rop->GetMeshData().m_IndexBufferData.m_indexCount = mCurrentSection->mIndexCount;
				rop->GetMeshData().m_IndexBufferData.m_indexBuffer = renderSystem->createStaticIB(mCurrentSection->mTempIndexBuffer->getPtr(), rop->GetMeshData().m_IndexBufferData.m_indexCount * sizeof(uint16));
			}
			// return the finished section
			result = mCurrentSection;
			mCurrentSection->touchMaterial();
			resetTempAreas();
			//mCurrentSection->clearData();
		} // empty section check

		
		//if ((mSectionList.size() == 0) ||
		//	((mSectionList[0][0]->mExistedVertexCount == mVertexCount) &&
		//	(mSectionList[0][0]->mExistedIndexCount == mIndexCount)))
		//resetTempAreas();
		mCurrentSection = 0;
		// Tell parent if present
		if (mParentNode)
		{
			mParentNode->needUpdate();
		}

		// will return the finished section or NULL if
		// the section was empty (i.e. zero vertices/indices)
		return result;
	}

	TrafficObject::TrafficObjectSection* TrafficObject::pre_end(void)
	{
		if (mTempVertexPending)
			mCurrentSection->mExistedVertexCount = mCurrentSection->mVertexCount + 1;
		else
			mCurrentSection->mExistedVertexCount = mCurrentSection->mVertexCount;
		mCurrentSection->mExistedIndexCount = mCurrentSection->mIndexCount;
		if ((mCurrentSection->mExistedVertexCount == mVertexCount) &&
			(mCurrentSection->mExistedIndexCount == mIndexCount))
		{
			mCurrentSection->mExistedVertexCount = 0;
			mCurrentSection->mExistedIndexCount = 0;
		}

		bool temp = mCurrentUpdating;
		if (mVertexCount != 0) mCurrentUpdating = true;
		TrafficObjectSection* result = end();
		if (mVertexCount != 0) mCurrentUpdating = temp;
		
		return result;
	}

	void TrafficObject::delay_end(void)
	{
		if (!delayEnd)
			return;
		for (SectionList& lodRender : mSectionList)
		{
			for (TrafficObjectSection* render : lodRender)
			{
				mCurrentSection = render;
				mDeclSize = TempVertexsizeGuess;
				mTempVertexPending = true;
				pre_end();
			}
		}
		delayEnd = false;
	}

	//-----------------------------------------------------------------------------
	MovableObjectType TrafficObject::getMovableType(void) const
	{
		DEF_THREAD_CHEEK(DEF_THREAD_TYPE_ALL)
			return MovableObjectType_ManualObject;
	}
	//-----------------------------------------------------------------------------
	const AxisAlignedBox& TrafficObject::getBoundingBox(void) const
	{
		DEF_THREAD_CHEEK(DEF_THREAD_TYPE_ALL)
			return mBounds;
	}
	//-----------------------------------------------------------------------------
	float TrafficObject::getBoundingRadius(void) const
	{
		DEF_THREAD_CHEEK(DEF_THREAD_TYPE_ALL)
			return mRadius;
	}
	void TrafficObject::_notifyCurrentCamera(Camera* cam)
	{
		getWorldBoundingBox(true);

		float distanceSquared = mWorldAABB.SquareDistPointToAABB(cam->getPosition());
		distanceSquared = std::max(0.000001f, distanceSquared);

		float tempRenderDistence = mRenderDistance;

		mCulled = (tempRenderDistence * tempRenderDistence) < distanceSquared;

		if (mCulled)
		{
			return;
		}

		if (cam->getCameraUsage() != SceneCamera)
			return;

		int lodLevel = LOD_LEVEL_NUM - 1;

		float fLodDisScale = DynamicVideoParamManager::instance()->getLodDisScale();
		fLodDisScale *= m_fGlobalLodScale;
		for (; lodLevel > LOD_LEVEL_0; --lodLevel)
		{
			float lodDistance = m_fLodDistance[lodLevel] * fLodDisScale;
			if (distanceSquared >= lodDistance * lodDistance)
				break;

			if (lodLevel != mCurrLodLevel)
			{
				if (lodLevel >= LOD_LEVEL_NUM)
				{
					lodLevel = LOD_LEVEL_NUM - 1;
				}
			}
		}

		if (!mSectionList.empty())
		{
			if (lodLevel >= mSectionList.size())
				lodLevel = (int)mSectionList.size() - 1;
			while (lodLevel >= 0 && mSectionList[lodLevel].empty())
			{
				lodLevel = lodLevel - 1;
			}
		}
		else
			lodLevel = 0;

		if (lodLevel != mCurrLodLevel)
		{
			mCurrLodLevel = (EM_LOD_LEVEL)lodLevel;
		}

	}
	//-----------------------------------------------------------------------------
	void TrafficObject::updateRenderQueue(RenderQueue* queue)
	{
		DEF_THREAD_CHEEK(DEF_THREAD_TYPE_ALL);
		if (mSectionList.empty())
			return;
		if (mCurrLodLevel >= mSectionList.size())
			mCurrLodLevel = (EM_LOD_LEVEL)(mSectionList.size() - 1);

		// Skip empty sections (only happens if non-empty first, then updated)
		SectionList& renderlist = mSectionList[mCurrLodLevel];
		for(auto& tos : renderlist)
		{
			//TrafficObjectSection* tos = mSectionList[mCurrLodLevel];
			RenderOperation* rop = tos->getRenderOperation();
			if (tos->mVertexCount == 0 || (rop->useIndexes && tos->mIndexCount == 0))
				continue;

			const Material* pMat = tos->getMaterial();
			if (!mCustomMaterial.isNull())
			{
				pMat = mCustomMaterial.get();
			}

			if (!tos->mUseDepthTest)
			{
				queue->getRenderQueueGroup(RenderQueue_EditorHelp)->addRenderable(pMat->getPass(NormalPass), tos, m_RenderPriority);
			}
			else
			{
				if (pMat->m_matInfo.alphaTestEnable)
					queue->getRenderQueueGroup(RenderQueue_SolidAlphaTest)->addRenderable(pMat->getPass(NormalPass), tos, m_RenderPriority);
				else if (pMat->m_desc.RenderState.BlendEnable)
					queue->getRenderQueueGroup(RenderQueue_Solid_Transparent)->addRenderable(pMat->getPass(NormalPass), tos, m_RenderPriority);
				else
					queue->getRenderQueueGroup(RenderQueue_Solid)->addRenderable(pMat->getPass(NormalPass), tos, m_RenderPriority);
			}
		}
	}


	//-----------------------------------------------------------------------------
	TrafficObject::TrafficObjectSection::TrafficObjectSection(
		TrafficObject* parent, const String& VSFileName, const String& PSFileName, ECHO_DRAW opType
	) : mParent(parent)
		, mShaderVSFileName(VSFileName)
		, mShaderPSFileName(PSFileName)
		, m_pVertexData(nullptr)
		, m_pIndexData(nullptr)
		, mUseDepthTest(true)
		, mUseVertexColor(false)
		, mUseDiffuseTexCoord(false)
		, mUseMaterialDiffuse(false)
		, m32BitIndices(false)
		, mUseDepthWrite(true)
		, mHadTouchMaterial(false)
		, mDelMaterial(true)
		, mTempVertexSize(TempInitialVertexSize)
		, mTempIndexSize(TempInitialIndexSize)
	{
		mRenderOperation.mPrimitiveType = opType;
		// default to no indexes unless we're told
		mRenderOperation.useIndexes = false;
		mMaterial = MaterialManager::instance()->createManual();
		mMaterial->m_desc.RenderState.CullMode = mCullMode;
		mMaterial->m_desc.RenderState.BlendEnable = ECHO_TRUE;
		mMaterial->m_desc.RenderState.BlendDst = ECHO_BLEND_INVSRCALPHA;
		mMaterial->m_desc.RenderState.BlendSrc = ECHO_BLEND_SRCALPHA;
		mMaterial->m_desc.RenderState.BlendOp = ECHO_BLEND_OP_ADD;
		mMaterial->m_desc.RenderState.DepthWriteEnable = ECHO_TRUE;
		mMaterial->m_desc.RenderState.DrawMode = opType;
		m_pIndexData = new IndexData();
		m_pVertexData = new VertexData();

		ShaderContentManager::instance()->GetOrLoadShaderContent(Name(mShaderVSFileName), mVSDX);
		ShaderContentManager::instance()->GetOrLoadShaderContent(Name(mShaderPSFileName), mPSDX);
	}

	TrafficObject::TrafficObjectSection::~TrafficObjectSection()
	{
		if (mDelMaterial)
			mMaterial = nullptr;

		SAFE_DELETE(m_pIndexData);
		SAFE_DELETE(m_pVertexData);
	}

	void TrafficObject::TrafficObjectSection::clearData()
	{
		if (m_pIndexData)
		{
			m_pIndexData->clearCpuStreamData();
		}
		if (m_pVertexData)
		{
			m_pVertexData->clearCpuStreamData();
		}
	}

	void TrafficObject::TrafficObjectSection::compressVertexData(int level)
	{
		const ECHO_IA_DESC &iaDesc = mMaterial->m_desc.IADesc;
		unsigned int iStreamStripSize[ECHO_IA_MAX_STREAM] = { 0 };
		ECHO_IA_ELEMENT newIAElem[ECHO_IA_MAX_ELEMENT];

		bool bCompress = false;

		for (unsigned e = 0; e < iaDesc.nElementCount; ++e)
		{
			bool bNeedCompress = false;
			newIAElem[e] = iaDesc.ElementArray[e];
			if (iaDesc.ElementArray[e].Semantics == ECHO_POSITION0 || iaDesc.ElementArray[e].Semantics == ECHO_TEXCOORD0 || iaDesc.ElementArray[e].Semantics == ECHO_TEXCOORD1)
			{
				if (iaDesc.ElementArray[e].eType == ECHO_FLOAT1 || iaDesc.ElementArray[e].eType == ECHO_FLOAT2) {
					newIAElem[e].eType = ECHO_FP16_VEC2;
					bNeedCompress = true;
				}
				else if (iaDesc.ElementArray[e].eType == ECHO_FLOAT3 || iaDesc.ElementArray[e].eType == ECHO_FLOAT4) {
					newIAElem[e].eType = ECHO_FP16_VEC4;
					bNeedCompress = true;
				}
			}
			else if (iaDesc.ElementArray[e].Semantics == ECHO_NORMAL0 || iaDesc.ElementArray[e].Semantics == ECHO_TANGENT0)
			{
				if (iaDesc.ElementArray[e].eType == ECHO_FLOAT3 || iaDesc.ElementArray[e].eType == ECHO_FLOAT4)
				{
					if (1 == level) {
						newIAElem[e].eType = ECHO_FP16_VEC4;
						bNeedCompress = true;
					}
					else if (2 == level) {
						newIAElem[e].eType = ECHO_UBYTE4N;
						bNeedCompress = true;
					}
				}
			}
			else if (iaDesc.ElementArray[e].Semantics == ECHO_COLOR0)
			{
				if (iaDesc.ElementArray[e].eType == ECHO_FLOAT3 || iaDesc.ElementArray[e].eType == ECHO_FLOAT4)
				{

					newIAElem[e].eType = ECHO_UBYTE4N;
					bNeedCompress = true;
				}
			}
			else if (iaDesc.ElementArray[e].Semantics == ECHO_BLENDWEIGHT0 || iaDesc.ElementArray[e].Semantics == ECHO_BLENDWEIGHT1)
			{
				if (iaDesc.ElementArray[e].eType == ECHO_FLOAT3 || iaDesc.ElementArray[e].eType == ECHO_FLOAT4)
				{
					if (1 == level) {
						newIAElem[e].eType = ECHO_FP16_VEC4;
						bNeedCompress = true;
					}
					else if (2 == level) {
						newIAElem[e].eType = ECHO_UBYTE4N;
						bNeedCompress = true;
					}
				}
			}
			bCompress |= bNeedCompress;
			if (!bNeedCompress)
			{
				newIAElem[e].eType = iaDesc.ElementArray[e].eType;
			}
			newIAElem[e].nOffset = iStreamStripSize[newIAElem[e].nStream];
			iStreamStripSize[iaDesc.ElementArray[e].nStream] += VertexType::typeSize(newIAElem[e].eType);
		}

		ECHO_IA_STREAM newIAStream[ECHO_IA_MAX_STREAM];
		for (unsigned int s = 0; s < iaDesc.nStreamCount; ++s)
		{
			newIAStream[s] = iaDesc.StreamArray[s];
			newIAStream[s].nStride = iStreamStripSize[s];
		}

		if (!bCompress)
		{
			return;
		}

		mVertexSize = newIAStream[0].nStride;

		VertexData *pNewVertexData = new VertexData;
		pNewVertexData->m_vertexCount = m_pVertexData->m_vertexCount;
		for (size_t sd = 0; sd < m_pVertexData->m_cpu_stream_data_list.size(); ++sd)
		{
			MemoryDataStreamPtr vtxDataStream(new MemoryDataStream(pNewVertexData->m_vertexCount * iStreamStripSize[sd]));
			pNewVertexData->addCpuStreamData(vtxDataStream->getPtr(), vtxDataStream,
				m_pVertexData->m_cpu_stream_data_list[sd]->m_bind_index,
				(unsigned short)iStreamStripSize[sd]);
		}

		for (unsigned int e = 0; e < iaDesc.nElementCount; ++e)
		{
			const ECHO_IA_ELEMENT &iaElem = iaDesc.ElementArray[e];
			const ECHO_IA_ELEMENT &iaNewElem = newIAElem[e];

			const VertexCpuStreamData *pSrcStreamData = m_pVertexData->GetVertexCpuDataBySource(iaElem.nStream);
			VertexCpuStreamData *pDstStreamData = pNewVertexData->GetVertexCpuDataBySource(iaNewElem.nStream);

			if (iaElem.Semantics == ECHO_POSITION0 || iaElem.Semantics == ECHO_TEXCOORD0 || iaDesc.ElementArray[e].Semantics == ECHO_TEXCOORD1)
			{
				for (unsigned int v = 0; v < pNewVertexData->m_vertexCount; ++v)
				{
					const unsigned char *pSrc = pSrcStreamData->mp_start_memory + v * pSrcStreamData->m_vertex_size + iaElem.nOffset;
					unsigned char *pDst = pDstStreamData->mp_start_memory + v * pDstStreamData->m_vertex_size + iaNewElem.nOffset;

					const float *pFloat = (const float*)pSrc;
					unsigned short *pUShort = (unsigned short *)pDst;
					pUShort[0] = pUShort[1] = 0u;
					if (iaNewElem.eType == ECHO_FP16_VEC4)
					{
						pUShort[2] = 0u;
						pUShort[3] = Math::f32_to_u16(1.f);
					}

					switch (iaElem.eType)
					{
					case ECHO_FLOAT4:		pUShort[3] = Math::f32_to_u16(pFloat[3]);
					case ECHO_FLOAT3:		pUShort[2] = Math::f32_to_u16(pFloat[2]);
					case ECHO_FLOAT2:		pUShort[1] = Math::f32_to_u16(pFloat[1]);
					case ECHO_FLOAT1:		pUShort[0] = Math::f32_to_u16(pFloat[0]);
						break;
					default:
						memcpy(pDst, pSrc, VertexType::typeSize(iaElem.eType));
						break;
					}
				}
			} // end of ECHO_POSITION0 || ECHO_TEXCOORD0
			else if (iaElem.Semantics == ECHO_NORMAL0 || iaElem.Semantics == ECHO_TANGENT0)
			{
				for (unsigned int v = 0; v < pNewVertexData->m_vertexCount; ++v)
				{
					const unsigned char *pSrc = pSrcStreamData->mp_start_memory + v * pSrcStreamData->m_vertex_size + iaElem.nOffset;
					unsigned char *pDst = pDstStreamData->mp_start_memory + v * pDstStreamData->m_vertex_size + iaNewElem.nOffset;

					if (iaElem.eType == ECHO_FLOAT3 || iaElem.eType == ECHO_FLOAT4)
					{
						const float *pFloat = (const float*)pSrc;

						if (iaNewElem.eType == ECHO_FP16_VEC4)
						{
							unsigned short *pUShort = (unsigned short*)pDst;
							pUShort[0] = Math::f32_to_u16(pFloat[0]);
							pUShort[1] = Math::f32_to_u16(pFloat[1]);
							pUShort[2] = Math::f32_to_u16(pFloat[2]);
							pUShort[3] = Math::f32_to_u16(0.f);
							if (iaElem.eType == ECHO_FLOAT4) {
								pUShort[3] = Math::f32_to_u16(pFloat[3]);
							}
						}
						else if (iaNewElem.eType == ECHO_UBYTE4N)
						{
							unsigned char *pUChar = (unsigned char *)pDst;
							Vector3 vNormal(pFloat);
							vNormal *= 0.5f;	vNormal += 0.5f;	vNormal *= 255.f;
							assert(vNormal.x >= 0 && vNormal.x <= 255.f &&
								vNormal.y >= 0 && vNormal.y <= 255.f &&
								vNormal.z >= 0 && vNormal.z <= 255.f);
							pUChar[0] = (unsigned char)vNormal.x;
							pUChar[1] = (unsigned char)vNormal.y;
							pUChar[2] = (unsigned char)vNormal.z;
							pUChar[3] = 0u;
							if (iaElem.eType == ECHO_FLOAT4)
							{
								float fValue = pFloat[3] * 255.f;
								if (fValue < 0.f)	fValue = 0.f;
								if (fValue > 255.f)	fValue = 255.f;
								pUChar[3] = (unsigned char)fValue;
							}
						}
						else
						{
							assert(iaNewElem.eType == iaElem.eType);
							if (iaNewElem.eType == iaElem.eType) {
								memcpy(pDst, pSrc, VertexType::typeSize(iaElem.eType));
							}
						}
					}
					else {
						memcpy(pDst, pSrc, VertexType::typeSize(iaElem.eType));
					}
				}
			} // end of ECHO_NORMAL0 || ECHO_TANGENT0
			else if (iaElem.Semantics == ECHO_COLOR0)
			{
				for (unsigned int v = 0; v < pNewVertexData->m_vertexCount; ++v)
				{
					const unsigned char *pSrc = pSrcStreamData->mp_start_memory + v * pSrcStreamData->m_vertex_size + iaElem.nOffset;
					unsigned char *pDst = pDstStreamData->mp_start_memory + v * pDstStreamData->m_vertex_size + iaNewElem.nOffset;

					if (iaElem.eType == ECHO_FLOAT3 || iaElem.eType == ECHO_FLOAT4)
					{
						const float *pFloat = (const float*)pSrc;
						if (iaNewElem.eType == ECHO_FP16_VEC4)
						{
							unsigned short *pUShort = (unsigned short*)pDst;
							pUShort[0] = Math::f32_to_u16(pFloat[0]);
							pUShort[1] = Math::f32_to_u16(pFloat[1]);
							pUShort[2] = Math::f32_to_u16(pFloat[2]);
							pUShort[3] = Math::f32_to_u16(0.f);
							if (iaElem.eType == ECHO_FLOAT4) {
								pUShort[3] = Math::f32_to_u16(pFloat[3]);
							}
						}
						else if (iaNewElem.eType == ECHO_UBYTE4N)
						{
							unsigned char *pUChar = (unsigned char *)pDst;
							Vector3 vColor;
							memcpy(vColor.ptr(), pFloat, sizeof(float) * 3);
							vColor *= 255.f;
							//assert(vColor.x >= 0 && vColor.x <= 255.f &&
							//	vColor.y >= 0 && vColor.y <= 255.f &&
							//	vColor.z >= 0 && vColor.z <= 255.f);

							for (uint8 i = 0; i < 3; ++i)
							{
								if (vColor[i] < 0)
									vColor[i] = 0;
								if (vColor[i] > 255)
									vColor[i] = 255;

								pUChar[i] = (unsigned char)vColor[i];
							}

							pUChar[0] = (unsigned char)vColor.x;
							pUChar[1] = (unsigned char)vColor.y;
							pUChar[2] = (unsigned char)vColor.z;
							pUChar[3] = 0u;
							if (iaElem.eType == ECHO_FLOAT4)
							{
								float fValue = pFloat[3] * 255.f;
								assert(fValue >= 0 && fValue <= 255.f);
								pUChar[3] = (unsigned char)fValue;
							}
						}
						else
						{
							assert(iaNewElem.eType == iaElem.eType);
							if (iaNewElem.eType == iaElem.eType) {
								memcpy(pDst, pSrc, VertexType::typeSize(iaElem.eType));
							}
						}
					}
					else {
						memcpy(pDst, pSrc, VertexType::typeSize(iaElem.eType));
					}
				}
			}
			else if (iaElem.Semantics == ECHO_BLENDWEIGHT0 || iaElem.Semantics == ECHO_BLENDWEIGHT1)
			{
				for (unsigned int v = 0; v < pNewVertexData->m_vertexCount; ++v)
				{
					const unsigned char *pSrc = pSrcStreamData->mp_start_memory + v * pSrcStreamData->m_vertex_size + iaElem.nOffset;
					unsigned char *pDst = pDstStreamData->mp_start_memory + v * pDstStreamData->m_vertex_size + iaNewElem.nOffset;

					if (iaElem.eType == ECHO_FLOAT3 || iaElem.eType == ECHO_FLOAT4)
					{
						const float *pFloat = (const float*)pSrc;
						if (iaNewElem.eType == ECHO_FP16_VEC4)
						{
							unsigned short *pUShort = (unsigned short*)pDst;
							pUShort[0] = Math::f32_to_u16(pFloat[0]);
							pUShort[1] = Math::f32_to_u16(pFloat[1]);
							pUShort[2] = Math::f32_to_u16(pFloat[2]);
							pUShort[3] = Math::f32_to_u16(0.f);
							if (iaElem.eType == ECHO_FLOAT4) {
								pUShort[3] = Math::f32_to_u16(pFloat[3]);
							}
						}
						else if (iaNewElem.eType == ECHO_UBYTE4N)
						{
							unsigned char *pUChar = (unsigned char *)pDst;
							Vector3 vWeight;
							memcpy(vWeight.ptr(), pFloat, sizeof(float) * 3);
							vWeight *= 255.f;
							//assert(vWeight.x >= 0 && vWeight.x <= 255.f &&
							//	vWeight.y >= 0 && vWeight.y <= 255.f &&
							//	vWeight.z >= 0 && vWeight.z <= 255.f);

							for (uint8 i = 0; i < 3; ++i)
							{
								if (vWeight[i] < 0)
									vWeight[i] = 0;
								if (vWeight[i] > 255)
									vWeight[i] = 255;

								pUChar[i] = (unsigned char)vWeight[i];
							}
							pUChar[3] = 0u;
							if (iaElem.eType == ECHO_FLOAT4)
							{
								float fValue = pFloat[3] * 255.f;
								assert(fValue >= 0 && fValue <= 255.f);
								pUChar[3] = (unsigned char)fValue;
							}
						}
						else
						{
							assert(iaNewElem.eType == iaElem.eType);
							if (iaNewElem.eType == iaElem.eType) {
								memcpy(pDst, pSrc, VertexType::typeSize(iaElem.eType));
							}
						}
					}
					else {
						memcpy(pDst, pSrc, VertexType::typeSize(iaElem.eType));
					}
				}
			}	// end of ECHO_BLENDWEIGHT0
			else
			{
				for (unsigned int v = 0; v < pNewVertexData->m_vertexCount; ++v)
				{
					const unsigned char *pSrc = pSrcStreamData->mp_start_memory + v * pSrcStreamData->m_vertex_size + iaElem.nOffset;
					unsigned char *pDst = pDstStreamData->mp_start_memory + v * pDstStreamData->m_vertex_size + iaNewElem.nOffset;
					memcpy(pDst, pSrc, VertexType::typeSize(iaElem.eType));
				}
			}
		}

		delete m_pVertexData;
		m_pVertexData = pNewVertexData;

		for (unsigned int e = 0; e < mMaterial->m_desc.IADesc.nElementCount; ++e)
			mMaterial->m_desc.IADesc.ElementArray[e] = newIAElem[e];

		for (unsigned int s = 0; s < mMaterial->m_desc.IADesc.nStreamCount; ++s)
			mMaterial->m_desc.IADesc.StreamArray[s] = newIAStream[s];
	}

	void TrafficObject::TrafficObjectSection::generateShaderMask()
	{
		mVSDXGenerateMask.clear();
		mPSDXGenerateMask.clear();
		if (mUseVertexColor)
		{
			mVSDXGenerateMask.append("#define VERTEX_COLOR\n");
			mPSDXGenerateMask.append("#define VERTEX_COLOR\n");
		}

		if (mUseDiffuseTexCoord)
		{
			mVSDXGenerateMask.append("#define DIFFUSE_TEX\n");
			mPSDXGenerateMask.append("#define DIFFUSE_TEX\n");
		}

		if (mUseMaterialDiffuse)
		{
			mPSDXGenerateMask.append("#define MATERIAL_DIFFUSE\n");
		}
	}

	void TrafficObject::TrafficObjectSection::touchMaterial()
	{
		if (mHadTouchMaterial)
		{
			return;
		}

		updateDescHash(mMaterial->m_desc.IADesc);

		if (mUseDepthTest)
		{
			mMaterial->m_desc.RenderState.DepthTestEnable = ECHO_TRUE;
		}
		else
		{
			mMaterial->m_desc.RenderState.DepthTestEnable = ECHO_FALSE;
		}

		mMaterial->m_desc.RenderState.DepthWriteEnable = mUseDepthWrite ? ECHO_TRUE : ECHO_FALSE;
		mMaterial->m_desc.RenderState.CullMode = mCullMode;

		mMaterial->m_desc.IADesc.StreamArray[0].nStride = mVertexSize;

		mMaterial->setShaderName(mShaderVSFileName, mShaderPSFileName);
		mMaterial->setShaderSrcCode(mVSDX, mPSDX);
		mMaterial->setMacroShaderSrcCode(mVSDXGenerateMask + mVSShaderMacro, mPSDXGenerateMask + mPSShaderMacro);
		mMaterial->createPass(NormalPass);
		mMaterial->loadInfo();

		mHadTouchMaterial = true;
	}

	void TrafficObject::TrafficObjectSection::setShaderMacro(
		const String& vsMacro, const String& psMacro)
	{
		mVSShaderMacro = vsMacro; mPSShaderMacro = psMacro;
	}

	void TrafficObject::TrafficObjectSection::setCullMode(ECHO_CULL_MODE mode)
	{
		mCullMode = mode;
	}

	void TrafficObject::TrafficObjectSection::setMaterial(const MaterialWrapper& material)
	{
		if (material.isV1())
		{
			Material* pmaterial = material.getV1();
			mMaterial->clone(pmaterial, true, false);

			memcpy(&mMaterial->m_desc.RenderState, &pmaterial->m_desc.RenderState, sizeof(pmaterial->m_desc.RenderState));

			mMaterial->setMaterialPassMask(pmaterial->getMaterialPassMask());
			//mMaterial->setMaterialPointShadowPassMask(pmaterial->getMaterialPointShadowPassMask());
			mMaterial->setMaterialRipplePassMask(pmaterial->getMaterialRipplePassMask());
			mMaterial->setMaterialSoftPassMask(pmaterial->getMaterialSoftPassMask());
		}
		else
		{
			MaterialV2* pMat = material.getV2();
			mMaterial->copyFromMaterialV2(pMat, nullptr, "", "", false, false);
		}


		uint32 elementSize = 0;
		mMaterial->m_desc.IADesc.ElementArray[0].nStream = 0;
		mMaterial->m_desc.IADesc.ElementArray[0].eType = ECHO_FLOAT3;
		mMaterial->m_desc.IADesc.ElementArray[0].Semantics = ECHO_POSITION0;
		mMaterial->m_desc.IADesc.ElementArray[0].nOffset = elementSize;
		elementSize += 3 * sizeof(float);
		mMaterial->m_desc.IADesc.ElementArray[1].nStream = 0;
		mMaterial->m_desc.IADesc.ElementArray[1].eType = ECHO_FLOAT3;
		mMaterial->m_desc.IADesc.ElementArray[1].Semantics = ECHO_NORMAL0;
		mMaterial->m_desc.IADesc.ElementArray[1].nOffset = elementSize;
		elementSize += 3 * sizeof(float);
		mMaterial->m_desc.IADesc.ElementArray[3].nStream = 0;
		mMaterial->m_desc.IADesc.ElementArray[3].eType = ECHO_FLOAT4;
		mMaterial->m_desc.IADesc.ElementArray[3].Semantics = ECHO_TANGENT0;
		mMaterial->m_desc.IADesc.ElementArray[3].nOffset = elementSize;
		elementSize += 4 * sizeof(float);
		mMaterial->m_desc.IADesc.ElementArray[2].nStream = 0;
		mMaterial->m_desc.IADesc.ElementArray[2].eType = ECHO_FLOAT2;
		mMaterial->m_desc.IADesc.ElementArray[2].Semantics = ECHO_TEXCOORD0;
		mMaterial->m_desc.IADesc.ElementArray[2].nOffset = elementSize;
		elementSize += 2 * sizeof(float);

		mMaterial->m_desc.IADesc.nElementCount = 4;
		mMaterial->m_desc.IADesc.nStreamCount = 1;
		mMaterial->m_desc.IADesc.StreamArray[0].nStride = elementSize;

		mDelMaterial = true;
	}

	//-----------------------------------------------------------------------------
	RenderOperation* TrafficObject::TrafficObjectSection::getRenderOperation(void)
	{
		DEF_THREAD_CHEEK(DEF_THREAD_TYPE_ALL)
			return &mRenderOperation;
	}
	//-----------------------------------------------------------------------------
	Material* TrafficObject::TrafficObjectSection::getMaterial() const
	{
		return mMaterial.get();
	}
	//-----------------------------------------------------------------------------
	const MaterialWrapper& TrafficObject::TrafficObjectSection::getMaterialWrapper() const
	{
		return Renderable::getMaterialWrapper();
	}
	//-----------------------------------------------------------------------------
	void TrafficObject::TrafficObjectSection::getWorldTransforms(DBMatrix4* xform) const
	{
		xform[0] = mParent->_getParentNodeFullTransform();
	}
	//-----------------------------------------------------------------------------
	void TrafficObject::TrafficObjectSection::getWorldTransforms(const DBMatrix4** xform) const
	{
		xform[0] = mParent->getWorldMatrix();
	}
}
