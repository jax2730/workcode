#include "EchoComponentStableHeaders.h"
#include "EchoCSGUtil.h"
#include <stack>
#include "EchoShaderContentManager.h"
#include "EchoMaterialManager.h"
#include "EchoTextureManager.h"

#include "EchoStringUtil.h"

#include "EchoSubMesh.h"
#include "EchoMeshMgr.h"

namespace Echo
{
	const float SCALE_EPSILON = 1e-5f;
	//----------------------------CSGNode----------------------------------

	CSGNode::CSGNode(vector<CSGPolygon>::type& polygons)
	{
		vector<CSGPolygon>::type tpolys = polygons;

		this->front = nullptr;
		this->back = nullptr;
		build(tpolys);
	}

	CSGNode::CSGNode()
	{
		this->back = nullptr;
		this->front = nullptr;
	}

	CSGNode::~CSGNode()
	{
		delete mPlane;

		mPlane = nullptr;
		this->back = nullptr;
		this->front = nullptr;
	}

	void CSGNode::build(vector<CSGPolygon>::type& polys)
	{
		if (polys.size() == 0)
			return;

		std::stack<CSGNode*> nodeStacks;
		std::stack<vector<CSGPolygon>::type*> polysStacks;

		nodeStacks.push(this);
		polysStacks.push(&polys);

		while (!nodeStacks.empty())
		{
			CSGNode* currentNode = nodeStacks.top();
			nodeStacks.pop();

			vector<CSGPolygon>::type* currentPolys = polysStacks.top();
			polysStacks.pop();

			if (currentPolys->size() == 0) return;

			if (currentNode->mPlane == nullptr)
			{
				currentNode->mPlane = new CSGPlane((*currentPolys)[0].plane);
			}

			vector<CSGPolygon>::type* tfront = new vector<CSGPolygon>::type();
			vector<CSGPolygon>::type* tback = new vector<CSGPolygon>::type();

			for (size_t i = 0; i < currentPolys->size(); i++)
			{
				currentNode->mPlane->splitPolygon((*currentPolys)[i], currentNode->polygons, currentNode->polygons, *tfront, *tback);
			}

			if (tback->size() > 0)
			{
				if (currentNode->back == nullptr)
				{
					currentNode->back = new CSGNode();
				}
				nodeStacks.push(currentNode->back);
				polysStacks.push(tback);
			}

			if (tfront->size() > 0)
			{
				if (currentNode->front == nullptr)
				{
					currentNode->front = new CSGNode();
				}
				nodeStacks.push(currentNode->front);
				polysStacks.push(tfront);
			}
		}
	}

	vector<CSGPolygon>::type& CSGNode::clipPolygons(vector<CSGPolygon>::type& polys)
	{
		if (polys.empty())
			return polys;

		if (this->mPlane == nullptr)
			return polys;

		std::stack<std::tuple<CSGNode*, std::vector<CSGPolygon>, std::vector<CSGPolygon>>> stackNodes;
		stackNodes.push(std::make_tuple(this, polys, polys));

		vector<CSGPolygon>::type* resultPolys = new vector<CSGPolygon>::type();

		while (!stackNodes.empty()) {
			auto [currentNode, front, back] = stackNodes.top();
			stackNodes.pop();

			if (currentNode->mPlane == nullptr) {
				if (currentNode->back != nullptr) {
					stackNodes.push(std::make_tuple(currentNode->back, back, std::vector<CSGPolygon>()));
				}

				if (currentNode->front != nullptr) {
					stackNodes.push(std::make_tuple(currentNode->front, front, back));
				}
				continue;
			}

			vector<CSGPolygon>::type* newFront = new vector<CSGPolygon>::type();
			vector<CSGPolygon>::type* newBack = new vector<CSGPolygon>::type();

			for (auto& poly : front) {
				currentNode->mPlane->splitPolygon(poly, *newFront, *newBack, *newFront, *newBack);
			}

			if (currentNode->back != nullptr) {
				stackNodes.push(std::make_tuple(currentNode->back, *newBack, std::vector<CSGPolygon>()));
			}
			else {
				resultPolys->insert(resultPolys->end(), newFront->begin(), newFront->end());
				delete newBack;
			}

			if (currentNode->front != nullptr) {
				stackNodes.push(std::make_tuple(currentNode->front, *newFront, *newFront));
			}
			else {
				resultPolys->insert(resultPolys->end(), newFront->begin(), newFront->end());
				delete newFront;
			}
		}

		return *resultPolys;
	}

	void CSGNode::invert()
	{
		if (this->polygons.size() == 0)
		{
			return;
		}

		for (int i = 0; i < this->polygons.size(); i++)
		{
			this->polygons[i].flip();
		}
		this->mPlane->Reverse();

		if (this->front != nullptr) {
			this->front->invert();
		}

		if (this->back != nullptr)
		{
			this->back->invert();
		}
		CSGNode* temp = this->front;
		this->front = this->back;
		this->back = temp;
	}

	/// <summary>
	/// treea->clipto(treeb)
	/// 从treea的所有节点中剔除掉位于节点plane背后的多边形。
	/// </summary>
	/// <param name="node"></param>
	void CSGNode::clipTo(CSGNode& node)
	{
		std::stack<CSGNode*> nodeStack;
		nodeStack.push(this);

		while (!nodeStack.empty())
		{
			CSGNode* popNode = nodeStack.top();
			nodeStack.pop();

			popNode->polygons = node.clipPolygons(popNode->polygons);

			if (popNode->back != nullptr) { nodeStack.push(popNode->back); }
			if (popNode->front != nullptr) { nodeStack.push(popNode->front); }
		}
	}

	void CSGNode::allPolygons(vector<CSGPolygon>::type& polys)
	{

		std::stack<CSGNode*> nodeStack;
		nodeStack.push(this);

		while (!nodeStack.empty())
		{
			CSGNode* node = nodeStack.top();
			nodeStack.pop();

			polys.insert(polys.end(), node->polygons.begin(), node->polygons.end());

			if (node->back != nullptr)
			{
				nodeStack.push(node->back);
			}
			if (node->front != nullptr)
			{
				nodeStack.push(node->front);
			}
		}
	}

	vector<CSGPolygon>::type& CSGNode::getpolygons()
	{
		return polygons;
	}

	void CSGNode::deleteTree(CSGNode* node)
	{
		if (node == nullptr)
			return;

		deleteTree(node->front);
		deleteTree(node->back);

		delete node;
	}

	//----------------------------CSGNode----------------------------------


	//----------------------------CSGBrush---------------------------
	CSGBrush::CSGBrush(vector<CSGPolygon>::type& polygons)
	{
		this->mPolygons = polygons;
	}

	void CSGBrush::configRenderInfo()
	{
		//renderable
		mMaterial = new Echo::Material(nullptr, Echo::Name(""));

		ECHO_IA_DESC& iaDesc = mMaterial->m_desc.IADesc;
		uint32 n = 0;
		iaDesc.ElementArray[n].nStream = 0;
		iaDesc.ElementArray[n].nOffset = offsetof(CSGDrawVertex, pos);
		iaDesc.ElementArray[n].eType = ECHO_FLOAT3;
		iaDesc.ElementArray[n].Semantics = ECHO_POSITION0;
		++n;

		iaDesc.ElementArray[n].nStream = 0;
		iaDesc.ElementArray[n].nOffset = offsetof(CSGDrawVertex, normal);
		iaDesc.ElementArray[n].eType = ECHO_FLOAT3;
		iaDesc.ElementArray[n].Semantics = ECHO_NORMAL0;
		++n;

		iaDesc.nElementCount = n;

		iaDesc.StreamArray[0].bPreInstance = ECHO_FALSE;
		iaDesc.StreamArray[0].nRate = 1;
		iaDesc.StreamArray[0].nStride = sizeof(CSGDrawVertex);
		iaDesc.nStreamCount = 1;

		mMaterial->getDiffuseColor() = Echo::ColorValue(0.5f, 0.5f, 0.5f, 1.0f);
		mMaterial->m_desc.RenderState.DepthTestEnable = ECHO_TRUE;
		mMaterial->m_desc.RenderState.BlendEnable = ECHO_TRUE;
		mMaterial->m_desc.RenderState.BlendSrc = ECHO_BLEND_SRCALPHA;
		mMaterial->m_desc.RenderState.BlendDst = ECHO_BLEND_INVSRCALPHA;
		mMaterial->m_desc.RenderState.BlendOp = ECHO_BLEND_OP_ADD;
		mMaterial->m_desc.RenderState.CullMode = ECHO_CULL_BACK;

		////material -------
		//std::string vsShader, psShader;
		//ShaderContentManager::instance()->GetOrLoadShaderContent(Name("Helper_VS.txt"), vsShader);
		//ShaderContentManager::instance()->GetOrLoadShaderContent(Name("Helper_PS.txt"), psShader);

		////adding macro definition to get illumination.
		//vsShader.insert(0, "#define ILLUM_BLINN\n\n");
		//psShader.insert(0, "#define ILLUM_BLINN\n\n#define MATERIAL_DIFFUSE\n\n");
		//mMaterial->setShaderSrcCode(vsShader, psShader);
		//mMaterial->createPass(NormalPass);
		//mMaterial->loadInfo();
		////-------

		//----ocean shader----
		std::string vsShader, psShader;
		ShaderContentManager::instance()->GetOrLoadShaderContent(Name("Ocean3_VS.txt"), vsShader);
		ShaderContentManager::instance()->GetOrLoadShaderContent(Name("Ocean3_PS.txt"), psShader);
		vsShader.insert(0, "#define USE_DIFFUSE_MAP\n\n");
		psShader.insert(0, "#define USE_DIFFUSE_MAP\n\n");
		mMaterial->setShaderName("Ocean3_VS.txt", "Ocean3_PS.txt");


		//MatInfo
		mMaterial->setShaderSrcCode(vsShader,psShader);
		mMaterial->createPass(NormalPass);
		mMaterial->loadInfo();

		//set texture
		TexturePtr diffTex = TextureManager::instance()->loadTexture(Name("model/obj/texture/gx_faguang_001_b.dds"), TEX_TYPE_2D);
		TexturePtr normalTex = TextureManager::instance()->loadTexture(Name("Textures/water/waves.dds"), TEX_TYPE_2D);
		TexturePtr amTex = TextureManager::instance()->loadTexture(Name("Textures/cube/gx_beishanhegu04.dds"), TEX_TYPE_CUBE_MAP);

		mMaterial->m_textureList[MAT_TEXTURE_DIFFUSE] = diffTex;
		mMaterial->m_textureList[MAT_TEXTURE_NORMAL] = normalTex;
		mMaterial->m_textureList[MAT_TEXTURE_REFLECTCUBE] = amTex;

		Vector3 shallowColor(0.494f, 0.482f, 0.419f);
		Vector3 deepColor(0.1372f, 0.101f, 0.054f);
		Vector3 foamColor(0.5137f, 0.5137f, 0.5137f);

		Echo::RenderSystem* rs = Root::instance()->getRenderSystem();
		rs->setUniformValue(this, mMaterial->getPass(NormalPass), U_PSGeneralRegister4, shallowColor);
		rs->setUniformValue(this, mMaterial->getPass(NormalPass), U_PSGeneralRegister5, deepColor);
		rs->setUniformValue(this, mMaterial->getPass(NormalPass), U_PSGeneralRegister6, foamColor);

		Vector4 vsparm(0.5f, 0.51f, 0.0f, 0.0f);
		rs->setUniformValue(this, mMaterial->getPass(NormalPass), U_VSGeneralRegister0, vsparm);

		Vector4 psparm(0.0f, 0.0f, 0.27f, 1.1f);
		rs->setUniformValue(this, mMaterial->getPass(NormalPass), U_PSGeneralRegister0, psparm);

		//----ocean shader----

		mRenderOp.mPrimitiveType = ECHO_TRIANGLE_LIST;

		//当前默认都是单位
		mBounds.setMinimum(Vector3(-0.5f, -0.5f, -0.5f));
		mBounds.setMinimum(Vector3(0.5f, 0.5f, 0.5f));

		char szNodeName[64] = { 0 };
		sprintf(szNodeName, "csgcube/%p", this);
		mNode = Root::instance()->getMainSceneManager()->createSceneNode(Name(szNodeName));
		mNode->attachObject(this);
	}

	CSGBrush::~CSGBrush()
	{
		if (mNode != nullptr)
		{
			mNode->detachAllObject();
			SceneNode* parentNode = mNode->getParentSceneNode();
			if (parentNode != nullptr)
			{
				parentNode->destroyChild(mNode);
			}
			mNode = nullptr;
		}

		delete mMaterial;
		mMaterial = nullptr;
	}

	CSGBrush* CSGBrush::mergeCSGBrush(CSGBrush* brush, CSGOperation operation)
	{
		switch (operation)
		{
		case eUnion:
			return this->unionCSGBrush(*brush);
			break;
		case eIntersection:
			return this->intersectCSGBrush(*brush);
			break;
		case eSbstruct:
			return this->subtractCSGBrush(*brush);
			break;
		default:
			return nullptr;
			break;
		}
	}

	CSGBrush* CSGBrush::unionCSGBrush(CSGBrush& csg)
	{
		CSGNode a(this->mPolygons);
		CSGNode b(csg.getPolygons());

		a.clipTo(b);
		b.clipTo(a);

		vector<CSGPolygon>::type polys;
		b.allPolygons(polys);
		a.build(polys);
		a.allPolygons(polys);

		return CSGBrush::buildFromPolygons(polys);
	}

	CSGBrush* CSGBrush::subtractCSGBrush(CSGBrush& csg)
	{
		CSGNode a(this->mPolygons);
		CSGNode b(csg.getPolygons());

		a.invert();
		a.clipTo(b);
		b.clipTo(a);
		b.invert();
		b.clipTo(a);
		b.invert();

		vector<CSGPolygon>::type polys;
		a.allPolygons(polys);
		b.allPolygons(polys);
		for (size_t i = 0; i < polys.size(); i++)
		{
			polys[i].flip();
		}

		return CSGBrush::buildFromPolygons(polys);
	}

	CSGBrush* CSGBrush::intersectCSGBrush(CSGBrush& csg)
	{
		CSGNode a(this->mPolygons);
		CSGNode b(csg.getPolygons());

		a.invert();
		b.invert();
		a.clipTo(b);
		b.clipTo(a);

		vector<CSGPolygon>::type polys;
		b.allPolygons(polys);
		a.build(polys);

		a.allPolygons(polys);

		CSGBrush* c = CSGBrush::buildFromPolygons(polys);
		c->inverse();

		return c;

	}

	vector<CSGPolygon>::type& CSGBrush::getPolygons()
	{
		return this->mPolygons;
	}

	void CSGBrush::inverse()
	{
		for (size_t i = 0; i < mPolygons.size(); i++)
		{
			mPolygons[i].flip();
		}
	}

	CSGBrush* CSGBrush::buildFromPolygons(vector<CSGPolygon>::type& polygons)
	{
		CSGBrush* brush = new CSGBrush(polygons);
		return brush;
	}

	void CSGBrush::setNodePosition(Vector3& pos)
	{
		mNode->_setDerivedPosition(pos);
	}

	void CSGBrush::updateBoundingBox()
	{
		Vector3 minPos = Vector3::MAX;
		Vector3 maxPos = Vector3::MIN;

		for (auto& iter : mPolygons)
		{
			vector<CSGVertex>::type vtxs = iter.getVertices();
			for (auto& vtx : vtxs)
			{
				minPos.x = Math::Min(vtx.mPosition.x, minPos.x);
				minPos.y = Math::Min(vtx.mPosition.y, minPos.y);
				minPos.z = Math::Min(vtx.mPosition.z, minPos.z);
				maxPos.x = Math::Max(vtx.mPosition.x, maxPos.x);
				maxPos.y = Math::Max(vtx.mPosition.y, maxPos.y);
				maxPos.z = Math::Max(vtx.mPosition.z, maxPos.z);
			}
		}
		mBounds.setExtents(minPos, maxPos);
	}

	void CSGBrush::generateDrawInfo()
	{
		vector<CSGPolygon>::type polygons = getPolygons();
		unsigned int polygonCount = static_cast<unsigned int>(polygons.size());
		vector<CSGPolygon>::type trisPolygons;

		m_VB.clear();
		m_IB.clear();
		mRenderOp.GetMeshData().Release();

		for ( int i = polygonCount - 1; i >=0 ; i--)
		{
			vector<CSGVertex>::type vertices = polygons[i].getVertices();
			unsigned int pvCount = static_cast<unsigned int>(vertices.size());

			if (pvCount > 3)
			{
				for (uint16 j = 1; j < pvCount - 1; j++)
				{
					vector<CSGVertex>::type tri;
					tri.push_back(vertices[0]);
					tri.push_back(vertices[j]);
					tri.push_back(vertices[j + 1]);

					trisPolygons.push_back(CSGPolygon(tri));
				}
				polygons.erase(polygons.begin() + i);
			}
		}
		polygons.insert(polygons.end(), trisPolygons.begin(), trisPolygons.end());
		//所有的polygon现在都变成了三角形。

		polygonCount = static_cast<int>(polygons.size());
		unsigned int vertexCount = polygonCount * 3;

		vector<CSGVertex>::type uniqueVertex;
		vector<uint16>::type uniqueVIndex;
		uniqueVIndex.resize(vertexCount);

		for (int pi = 0; pi < polygons.size(); pi++)
		{
			CSGPolygon poly = polygons[pi];

			for (int vi = 0; vi < 3; vi++)
			{
				CSGVertex v = poly.getVertices()[vi];

				int uniqueCount = static_cast<int>(uniqueVertex.size());

				bool isUniqueVertex = true;
				for (int i = 0; i < uniqueCount; i++)
				{
					CSGVertex uniqueV = uniqueVertex[i];
					if (uniqueV.mPosition.isEqual(v.mPosition) &&
						uniqueV.mNormal.isEqual(v.mNormal))
					{
						v.index = uniqueV.index;
						isUniqueVertex = false;
						break;
					}
				}

				if (isUniqueVertex)
				{
					uniqueCount = static_cast<int>(uniqueVertex.size());
					v.index = uniqueCount;
					uniqueVertex.push_back(v);
				}

				uniqueVIndex[pi * 3 + vi] = v.index;
			}
		}

		Vector3 minPos;
		Vector3 maxPos;

		for (auto& iter : uniqueVertex)
		{
			CSGDrawVertex vtx;
			vtx.pos = iter.mPosition;
			vtx.normal = iter.mNormal;
			m_VB.push_back(vtx);

			minPos.x = Math::Min(iter.mPosition.x, minPos.x);
			minPos.y = Math::Min(iter.mPosition.y, minPos.y);
			minPos.z = Math::Min(iter.mPosition.z, minPos.z);
			maxPos.x = Math::Max(iter.mPosition.x, maxPos.x);
			maxPos.y = Math::Max(iter.mPosition.y, maxPos.y);
			maxPos.z = Math::Max(iter.mPosition.z, maxPos.z);
		}
		mBounds.setExtents(minPos, maxPos);

		for (auto& iter : uniqueVIndex)
		{
			m_IB.push_back(iter);
		}
	}

	void CSGBrush::resetRender()
	{
		if (mNode == nullptr)
			return;
		mNode->needUpdateBounds();
		mNode->_updateBounds();
		if (m_VB.empty() || m_IB.empty())
			return;

		RenderSystem* pRenderSystem = Root::instance()->getRenderSystem();
		mRenderOp.GetMeshData().m_IndexBufferData.m_indexCount = uint32(m_IB.size());
		mRenderOp.GetMeshData().m_IndexBufferData.m_indexBuffer = pRenderSystem->createStaticIB(&m_IB[0], uint32(m_IB.size()) * sizeof(uint16));
		mRenderOp.GetMeshData().m_vertexCount = uint32(m_VB.size());
		mRenderOp.GetMeshData().m_vertexBuffer.push_back(pRenderSystem->createStaticVB(&m_VB[0], uint32(m_VB.size()) * sizeof(CSGDrawVertex)));
	}

	bool CSGBrush::exportToMesh(const Echo::String& meshfilepath,const Echo::String& templateMatPath)
	{

		std::string meshName = meshfilepath.substr(Echo::Root::instance()->getRootResourcePath().length());
		Echo::MeshPtr meshPtr = Echo::MeshMgr::instance()->createResource(Echo::Name(meshName.c_str()));

		std::string matName = templateMatPath.substr(Echo::Root::instance()->getRootResourcePath().length());

		//load template material
		MaterialPtr templateMat = MaterialManager::instance()->load(Echo::Name(matName));
		if (templateMat.isNull())
		{
			//material load failed,just return.
			return false;
		}

		//pSubMesh
		Echo::SubMesh* pFrontSubMesh = meshPtr->createSubMesh();
		pFrontSubMesh->setMaterial(templateMat);
		pFrontSubMesh->setMaterialName(Echo::Name(matName));
		pFrontSubMesh->setMatInfoName(Echo::Name(templateMat.get()->m_matInfo.name));

		this->convertToSubMesh(pFrontSubMesh);

		//backface subMesh
		Echo::SubMesh* pBackSubMesh = meshPtr->createSubMesh();
		pBackSubMesh->setMaterial(templateMat);
		pBackSubMesh->setMaterialName(Echo::Name(matName));
		pBackSubMesh->setMatInfoName(Echo::Name(templateMat.get()->m_matInfo.name));
		std::reverse(m_IB.begin(), m_IB.end());
		this->convertToSubMesh(pBackSubMesh);

		meshPtr->setAABB(mBounds.getMinimum(),mBounds.getMaximum());
		meshPtr->m_bUseLod = false;

		meshPtr->exportToBinary(meshName.c_str(),false,false);
		return true;
	}

	bool CSGBrush::exportToMesh(const Echo::String& meshfilepath, const Echo::String& outerTemplateMatPath, const Echo::String& innerTemplateMatPath)
	{
		if (meshfilepath.empty() || outerTemplateMatPath.empty() || innerTemplateMatPath.empty())
		{
			return false;
		}

		std::string meshName = meshfilepath.substr(Echo::Root::instance()->getRootResourcePath().length());

		bool invaliedMeshName = MaterialManager::instance()->isInvalidRes(Echo::Name(meshName));
		if (invaliedMeshName)
		{
			return false;
		}

		Echo::MeshPtr meshPtr = Echo::MeshMgr::instance()->createResource(Echo::Name(meshName.c_str()));

		std::string outerMatName = outerTemplateMatPath.substr(Echo::Root::instance()->getRootResourcePath().length());
		std::string innerMatName = innerTemplateMatPath.substr(Echo::Root::instance()->getRootResourcePath().length());

		bool invaliedOuterMat = MaterialManager::instance()->isInvalidRes(Echo::Name(outerMatName));
		bool invaliedInnerMat = MaterialManager::instance()->isInvalidRes(Echo::Name(innerMatName));

		if (invaliedOuterMat || invaliedInnerMat)
		{
			return false;
		}

		//outer subMesh
		MaterialPtr outerMatPtr = MaterialManager::instance()->load(Echo::Name(outerMatName));
		Echo::SubMesh* pOuterSubMesh = meshPtr->createSubMesh();
		pOuterSubMesh->setMaterial(outerMatPtr);
		pOuterSubMesh->setMaterialName(outerMatPtr->getName());
		pOuterSubMesh->setMatInfoName(Echo::Name(outerMatPtr->m_matInfo.name));
		pOuterSubMesh->mMatIndex = 0;
		this->convertToSubMesh(pOuterSubMesh);

		//inner subMesh
		std::reverse(m_IB.begin(), m_IB.end()); //内部的面是反的，直接把Index倒着绕一遍。

		MaterialPtr innerMatPtr = MaterialManager::instance()->load(Echo::Name(innerMatName));
		Echo::SubMesh* pInnerSubMesh = meshPtr->createSubMesh();
		pInnerSubMesh->setMaterial(innerMatPtr);
		pInnerSubMesh->setMaterialName(innerMatPtr->getName());
		pInnerSubMesh->setMatInfoName(Echo::Name(innerMatPtr->m_matInfo.name));
		pInnerSubMesh->mMatIndex = 1;
		this->convertToSubMesh(pInnerSubMesh);

		meshPtr->setAABB(mBounds.getMinimum(), mBounds.getMaximum());
		meshPtr->m_bUseLod = false;

		meshPtr->exportToBinary(meshName.c_str(), false, false);
		return true;
	}

	void CSGBrush::convertToSubMesh(Echo::SubMesh* pSubMesh)
	{
		unsigned int vtxSize = 0;
		vtxSize += sizeof(CSGDrawVertex); //Echo_FLOAT3 * 2  position + normal = 12 * 2

		unsigned int vtxCount = static_cast<unsigned int>(m_VB.size());
		Echo::MemoryDataStreamPtr vtxDataStream(new Echo::MemoryDataStream(vtxSize * vtxCount));

		void* vtxBuff = malloc(vtxSize * vtxCount);
		memcpy(vtxBuff, m_VB.data(), sizeof(CSGDrawVertex) * vtxCount);

		//write vtx data
		for (int i = 0; i < m_VB.size(); i++)
		{
			vtxDataStream->write((unsigned char*)vtxBuff + i * vtxSize, vtxSize);
		}

		vtxDataStream->seek(0);

		pSubMesh->getVertexData()->addCpuStreamData(vtxDataStream->getCurrentPtr(), vtxDataStream, 0, vtxSize);
		pSubMesh->getVertexData()->m_vertexCount = vtxCount;

		size_t uintSize = sizeof(unsigned short);
		Echo::MemoryDataStreamPtr idxDataStream(new Echo::MemoryDataStream(m_IB.size()* uintSize));

		memcpy(idxDataStream->getCurrentPtr(), m_IB.data(), m_IB.size() * uintSize);

		pSubMesh->getIndexData()->setCpuStreamData(idxDataStream->getCurrentPtr(), idxDataStream);
		pSubMesh->getIndexData()->m_indexCount = static_cast<uint32>(m_IB.size());


		const MaterialWrapper& matWrapper = pSubMesh->getMaterialWrapper();
		if (matWrapper.isV1())
		{
			Echo::Material* pMat = matWrapper.getV1();
			/*	pMat->m_matInfo.name = matName;
				pMat->m_matInfo.shadername = "Ocean3";
				pSubMesh->setMatInfoName(Echo::Name(matName));*/
			Echo::ECHO_IA_DESC& iaDesc = pMat->m_desc.IADesc;
			//Echo::ECHO_IA_DESC& iaDesc = pSubMesh->m_desc.IADesc;
			iaDesc.nElementCount = 0;

			uint32 n = 0;
			iaDesc.ElementArray[n].nStream = 0;
			iaDesc.ElementArray[n].nOffset = offsetof(CSGDrawVertex, pos);
			iaDesc.ElementArray[n].eType = ECHO_FLOAT3;
			iaDesc.ElementArray[n].Semantics = ECHO_POSITION0;
			++n;

			iaDesc.ElementArray[n].nStream = 0;
			iaDesc.ElementArray[n].nOffset = offsetof(CSGDrawVertex, normal);
			iaDesc.ElementArray[n].eType = ECHO_FLOAT3;
			iaDesc.ElementArray[n].Semantics = ECHO_NORMAL0;
			++n;

			iaDesc.nElementCount = n;

			iaDesc.StreamArray[0].bPreInstance = ECHO_FALSE;
			iaDesc.StreamArray[0].nRate = 1;
			iaDesc.StreamArray[0].nStride = sizeof(CSGDrawVertex);
			iaDesc.nStreamCount = 1;

			memcpy(&pSubMesh->m_desc.IADesc, &pMat->m_desc.IADesc, sizeof(pMat->m_desc.IADesc));
		}
		else if (matWrapper.isV2())
		{
			MaterialV2* pMat = matWrapper.getV2();
			ECHO_IA_DESC iaDesc = pMat->getOriginalVertexInputAttributeDesc();
			iaDesc.nElementCount = 0;

			uint32 n = 0;
			iaDesc.ElementArray[n].nStream = 0;
			iaDesc.ElementArray[n].nOffset = offsetof(CSGDrawVertex, pos);
			iaDesc.ElementArray[n].eType = ECHO_FLOAT3;
			iaDesc.ElementArray[n].Semantics = ECHO_POSITION0;
			++n;

			iaDesc.ElementArray[n].nStream = 0;
			iaDesc.ElementArray[n].nOffset = offsetof(CSGDrawVertex, normal);
			iaDesc.ElementArray[n].eType = ECHO_FLOAT3;
			iaDesc.ElementArray[n].Semantics = ECHO_NORMAL0;
			++n;

			iaDesc.nElementCount = n;

			iaDesc.StreamArray[0].bPreInstance = ECHO_FALSE;
			iaDesc.StreamArray[0].nRate = 1;
			iaDesc.StreamArray[0].nStride = sizeof(CSGDrawVertex);
			iaDesc.nStreamCount = 1;

			pMat->setVertexInputAttributeDesc(iaDesc);
			memcpy(&pSubMesh->m_desc.IADesc, &iaDesc, sizeof(ECHO_IA_DESC));
		}
		else
		{
			assert(false);
		}
	}

	//---override MovableObject---
	Echo::MovableObjectType CSGBrush::getMovableType() const
	{
		return Echo::MovableObjectType_SimpleRenderable;
	}

	const Echo::AxisAlignedBox& CSGBrush::getBoundingBox() const
	{
		return mBounds;
	}

	float CSGBrush::getBoundingRadius() const
	{
		return Math::boundingRadiusFromAABB(mBounds);
	}

	void CSGBrush::updateRenderQueue(Echo::RenderQueue* pRenderQueue)
	{
		RenderQueueGroup* pGroup = pRenderQueue->getRenderQeueuGroup(RenderQueue_EditorHelp);
		pGroup->addRenderable(mMaterial->getPass(Echo::NormalPass), this);
	}
	//---override MovableObject---

	//---override Renderable---
	void CSGBrush::getWorldTransforms(Echo::DBMatrix4* xform) const
	{
		*xform = *getWorldMatrix();
	}

	void CSGBrush::getWorldTransforms(const Echo::DBMatrix4** xform) const
	{
		*xform = getWorldMatrix();
	}

	RenderOperation* CSGBrush::getRenderOperation()
	{
		return &mRenderOp;
	}

	// const Echo::Material* CSGBrush::getMaterial() const
	// {
	// 	return mMaterial;
	// }

	const MaterialWrapper& CSGBrush::getMaterialWrapper() const
	{
		if (m_materialWrapper.getV1() != mMaterial)
		{
			m_materialWrapper = MaterialWrapper::fromPointer(mMaterial);
		}
		return m_materialWrapper;
	}

	//---override Renderable---

	CSGPolygon::CSGPolygon()
	{
	}

	CSGPolygon::CSGPolygon(vector<CSGVertex>::type pVertices)
	{
		this->vertices = pVertices;

		Vector3 edge1 = vertices[1].mPosition - vertices[0].mPosition;
		Vector3 edge2 = vertices[2].mPosition - vertices[0].mPosition;

		Vector3 normal = edge1.crossProduct(edge2);
		normal.normalise();
		float d = normal.dotProduct(vertices[0].mPosition);

		this->plane = Plane(normal.x, normal.y, normal.z, d);
	}

	CSGPolygon::~CSGPolygon() {}

	void CSGPolygon::flip()
	{
		std::reverse(vertices.begin(), vertices.end());
		for (size_t i = 0; i < vertices.size(); i++)
		{
			vertices[i].mNormal *= -1.0f;
		}

		this->plane.Reverse();
	}

	vector<CSGVertex>::type& CSGPolygon::getVertices()
	{
		return vertices;
	}

	//-------------------------CSGPlane---------------------------
	enum PLANE_SIDE_TYPE
	{
		PLANESIDE_COPLANAR,
		PLANESIDE_FRONT,
		PLANESIDE_BACK,
		PLANESIDE_SPANNING,
	};

	CSGPlane::CSGPlane(Plane& other)
	{
		normal = other.normal;
		d = other.d;
	}

	//判断两个面的关系，如果跨骑就进行分割。
	void CSGPlane::splitPolygon(CSGPolygon& polygon,
		Echo::vector<CSGPolygon>::type& coplanarFront,
		Echo::vector<CSGPolygon>::type& coplanarBack,
		Echo::vector<CSGPolygon>::type& front,
		Echo::vector<CSGPolygon>::type& back)
	{
		vector<CSGVertex>::type vertex = polygon.getVertices();

		unsigned int polyT = 0;
		Echo::vector<int>::type polyTs;

		unsigned int type;
		float dis;

		for (size_t i = 0; i < vertex.size(); i++)
		{
			dis = this->normal.Dot(vertex[i].mPosition) - this->d;

			if (dis < -SCALE_EPSILON) { type = PLANESIDE_BACK; }
			else if (dis > SCALE_EPSILON) { type = PLANESIDE_FRONT; }
			else { type = PLANESIDE_COPLANAR; }

			polyT |= type;
			polyTs.push_back(type);
		}

		switch (polyT)
		{
		case PLANESIDE_COPLANAR:
			if (this->normal.Dot(polygon.plane.normal) > 0.0f)
			{
				coplanarFront.push_back(polygon);
			}
			else
			{
				coplanarBack.push_back(polygon);
			}
			break;

		case PLANESIDE_FRONT:
			front.push_back(polygon);
			break;
		case PLANESIDE_BACK:
			back.push_back(polygon);
			break;
		case PLANESIDE_SPANNING:
		{
			vector<CSGVertex>::type* frontP = new vector<CSGVertex>::type();
			vector<CSGVertex>::type* backP = new vector<CSGVertex>::type();

			for (int i = 0; i < vertex.size(); i++)
			{
				int j = (i + 1) % vertex.size();
				int ti = polyTs[i];
				int tj = polyTs[j];

				if (ti != PLANESIDE_BACK) { frontP->push_back(CSGVertex(vertex[i].mPosition, vertex[i].mNormal)); }
				if (ti != PLANESIDE_FRONT) { backP->push_back(ti != PLANESIDE_BACK ? CSGVertex(vertex[i].mPosition, vertex[i].mNormal) : vertex[i]); }
				if ((ti | tj) == PLANESIDE_SPANNING)
				{
					dis = (this->d - normal.Dot(vertex[i].mPosition)) /
						normal.Dot(vertex[j].mPosition - vertex[i].mPosition);

					Vector3 vpos = Vector3::lerp(vertex[i].mPosition, vertex[j].mPosition, dis);
					Vector3 vnormal = Vector3::lerp(vertex[i].mNormal, vertex[j].mNormal, dis);
					CSGVertex* v = new CSGVertex(vpos, vnormal);

					frontP->push_back(*v);
					backP->push_back(CSGVertex(vpos, vnormal));
				}
			}
			if (frontP->size() >= 3)
			{
				front.push_back(CSGPolygon(*frontP));
			}
			if (backP->size() >= 3)
			{
				back.push_back(CSGPolygon(*backP));
			}
			delete frontP;
			delete backP;
		}
		break;
		}
	}

	//-------------------------CSGShape---------------------------
	void CSGShape::makeSphereVertex(vector<CSGVertex>::type& vertex, const Vector3& center, float r, float theta, float phi,const Matrix3& msr,const Matrix3& mr)
	{
		theta *= Math::PI * 2;
		phi *= Math::PI;
		float cosTheta = Math::Cos(theta);
		float sinTheta = Math::Sin(theta);
		float cosPhi   = Math::Cos(phi);
		float sinPhi   = Math::Sin(phi);
		//球坐标-->顶点转换公式。
		Vector3 dir = Vector3(cosTheta * sinPhi,
			cosPhi,
			sinTheta * sinPhi);

		Vector3 sdir(dir * r);

		vertex.push_back(CSGVertex(msr * sdir + center, mr * dir));
	}

	CSGBrush* CSGShape::sphereCSG(const Vector3& center, const Vector3& eulangle, const Vector3& scale,  float radius, float slices, float stacks)
	{
		float r = radius;
		vector<CSGPolygon>::type polygons;
		vector<CSGVertex>::type vertices;

		Matrix3 ms = Matrix3::IDENTITY;
		ms._11 = scale.x ;
		ms._22 = scale.y ;
		ms._33 = scale.z ;

		Matrix3 mr;
		Radian radx(Math::DegreesToRadians(eulangle.x));
		Radian rady(Math::DegreesToRadians(eulangle.y));
		Radian radz(Math::DegreesToRadians(eulangle.z));

		mr.FromEulerAnglesXYZ(radx, rady, radz);

		Matrix3 mrs = mr * ms;

		for (int i = 0; i < slices; i++)
		{
			for (int j = 0; j < stacks; j++)
			{
				vertices.clear();
				makeSphereVertex(vertices, center, r, i / slices, j / stacks, mrs, mr);
				if (j > 0)
				{
					makeSphereVertex(vertices, center, r, (i + 1) / slices, j / stacks, mrs, mr);
				}
				if (j < stacks - 1)
				{
					makeSphereVertex(vertices, center, r, (i + 1) / slices, (j + 1) / stacks, mrs, mr);
				}
				makeSphereVertex(vertices, center, r, i / slices, (j + 1) / stacks, mrs, mr);
				polygons.push_back(CSGPolygon(vertices));
			}
		}

		CSGBrush* brush = new CSGBrush(polygons);

		return brush;
	}

	CSGBrush* CSGShape::cubeCSG(const Vector3& center, const Vector3& eulangle, const Vector3& scale, const Vector3& halfExtents)
	{
		vector<CSGPolygon>::type polygons;
		vector<Vector3>::type vertices;
		vector<Vector3>::type normal;

		Matrix3 ms = Matrix3::IDENTITY;
		ms._11 *= scale.x * halfExtents.x;
		ms._22 *= scale.y * halfExtents.y;
		ms._33 *= scale.z * halfExtents.z;

		Matrix3 mr;

		Radian radx(Math::DegreesToRadians(eulangle.x));
		Radian rady(Math::DegreesToRadians(eulangle.y));
		Radian radz(Math::DegreesToRadians(eulangle.z));

		mr.FromEulerAnglesXYZ(radx, rady, radz);

		Matrix3 m3 = mr * ms;

		vertices.push_back(m3 * Vector3(-1.f, -1.f, -1.f) + center);
		vertices.push_back(m3 * Vector3(-1.f, -1.f, 1.f)  + center);
		vertices.push_back(m3 * Vector3(1.f, -1.f, 1.f)   + center);
		vertices.push_back(m3 * Vector3(1.f, -1.f, -1.f)  + center);
		vertices.push_back(m3 * Vector3(-1.f, 1.f, -1.f)  + center);
		vertices.push_back(m3 * Vector3(-1.f, 1.f, 1.f)   + center);
		vertices.push_back(m3 * Vector3(1.f, 1.f, 1.f)    + center);
		vertices.push_back(m3 * Vector3(1.f, 1.f, -1.f)   + center);

		normal.push_back(mr * Vector3(0.0f, -1.f, 0.0f));
		normal.push_back(mr * Vector3(0.0f, 1.f, 0.0f));
		normal.push_back(mr * Vector3(-1.0f, 0.f, 0.0f));
		normal.push_back(mr * Vector3(0.0f, 0.f, 1.0f));
		normal.push_back(mr * Vector3(1.0f, 0.f, 0.0f));
		normal.push_back(mr * Vector3(0.0f, 0.f, -1.0f));

		vector<CSGVertex>::type bottom;
		bottom.push_back(CSGVertex(vertices[0], normal[0]));
		bottom.push_back(CSGVertex(vertices[3], normal[0]));
		bottom.push_back(CSGVertex(vertices[2], normal[0]));
		bottom.push_back(CSGVertex(vertices[1], normal[0]));
		polygons.push_back(CSGPolygon(bottom));

		vector<CSGVertex>::type top;
		top.push_back(CSGVertex(vertices[4], normal[1]));
		top.push_back(CSGVertex(vertices[5], normal[1]));
		top.push_back(CSGVertex(vertices[6], normal[1]));
		top.push_back(CSGVertex(vertices[7], normal[1]));
		polygons.push_back(CSGPolygon(top));

		vector<CSGVertex>::type left;
		left.push_back(CSGVertex(vertices[0], normal[2]));
		left.push_back(CSGVertex(vertices[1], normal[2]));
		left.push_back(CSGVertex(vertices[5], normal[2]));
		left.push_back(CSGVertex(vertices[4], normal[2]));
		polygons.push_back(CSGPolygon(left));

		vector<CSGVertex>::type front;
		front.push_back(CSGVertex(vertices[1], normal[3]));
		front.push_back(CSGVertex(vertices[2], normal[3]));
		front.push_back(CSGVertex(vertices[6], normal[3]));
		front.push_back(CSGVertex(vertices[5], normal[3]));
		polygons.push_back(CSGPolygon(front));

		vector<CSGVertex>::type right;
		right.push_back(CSGVertex(vertices[2], normal[4]));
		right.push_back(CSGVertex(vertices[3], normal[4]));
		right.push_back(CSGVertex(vertices[7], normal[4]));
		right.push_back(CSGVertex(vertices[6], normal[4]));
		polygons.push_back(CSGPolygon(right));

		vector<CSGVertex>::type back;
		back.push_back(CSGVertex(vertices[3], normal[5]));
		back.push_back(CSGVertex(vertices[0], normal[5]));
		back.push_back(CSGVertex(vertices[4], normal[5]));
		back.push_back(CSGVertex(vertices[7], normal[5]));
		polygons.push_back(CSGPolygon(back));

		CSGBrush* brush = new CSGBrush(polygons);
		return brush;
	}

	CSGBrush* CSGShape::cylinderCSG(const Vector3& center, const Vector3& eulangle, const Vector3& scale,  float radius, float halfHeight, float side)
	{
		vector<CSGPolygon>::type polygons;
		vector<CSGVertex>::type vertices;
		vector<Vector3>::type vertex;

		Matrix3 ms = Matrix3::IDENTITY;
		ms._11 *= scale.x;
		ms._22 *= scale.y;
		ms._33 *= scale.z;

		Matrix3 mr;

		Radian radx(Math::DegreesToRadians(eulangle.x));
		Radian rady(Math::DegreesToRadians(eulangle.y));
		Radian radz(Math::DegreesToRadians(eulangle.z));

		mr.FromEulerAnglesXYZ(radx, rady, radz);

		Matrix3 m3 = mr * ms;

		float rad = 1.0f / side * Math::PI * 2.0f;

		for (int i = 0; i < side; i++)
		{
			float crad = i * rad;
			float nrad = (i + 1) * rad;

			float cxpos = Math::Cos(crad);
			float czpos = Math::Sin(crad);
			float nxpos = Math::Cos(nrad);
			float nzpos = Math::Sin(nrad);

			vertex.clear();

			vertex.push_back(m3 * Vector3(cxpos * radius, halfHeight, czpos * radius) +center);
			vertex.push_back(m3 * Vector3(cxpos * radius, -halfHeight, czpos * radius)+center);
			vertex.push_back(m3 * Vector3(nxpos * radius, halfHeight, nzpos * radius) +center);
			vertex.push_back(m3 * Vector3(nxpos * radius, -halfHeight, nzpos * radius)+center);

			//sideface
			Vector3 currNormal = mr * Vector3(cxpos, 0.0f, czpos);
			Vector3 nextNormal = mr * Vector3(nxpos, 0.0f, nzpos);
			vector<CSGVertex>::type sideface;
			sideface.push_back(CSGVertex(vertex[0], currNormal));
			sideface.push_back(CSGVertex(vertex[2], nextNormal));
			sideface.push_back(CSGVertex(vertex[3], currNormal));
			sideface.push_back(CSGVertex(vertex[1], nextNormal));

			polygons.push_back(CSGPolygon(sideface));
			sideface.clear();

			//top
			Vector3 topNormal = mr * Vector3(0.0f, 1.0f, 0.0f);
			sideface.push_back(CSGVertex(vertex[0], topNormal));
			sideface.push_back(CSGVertex(m3 * Vector3(0.0f, halfHeight, 0.0f) + center, topNormal));
			sideface.push_back(CSGVertex(vertex[2], topNormal));
			polygons.push_back(CSGPolygon(sideface));
			sideface.clear();

			//bottom
			Vector3 bottomNormal = mr * Vector3(0.0f, -1.0f, 0.0f);
			sideface.push_back(CSGVertex(vertex[3], bottomNormal));
			sideface.push_back(CSGVertex(m3 * Vector3(0.0f, -halfHeight, 0.0f)+ center, bottomNormal));
			sideface.push_back(CSGVertex(vertex[1], bottomNormal));
			polygons.push_back(CSGPolygon(sideface));
		}

		CSGBrush* brush = new CSGBrush(polygons);
		return brush;
	}
}
