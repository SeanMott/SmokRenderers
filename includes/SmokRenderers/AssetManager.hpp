#pragma once

//asset manager for all renderers

#include <BTDSTD/Maps/IDHash.hpp>

#include <SmokMesh/Mesh.hpp>
#include <SmokMesh/MegaMeshBuffer.hpp>

#include <SmokTexture/Texture.hpp>
#include <SmokTexture/TextureBuffer.hpp>

#include <SmokGraphics/Pipeline/GraphicsPipeline.hpp>

namespace Smok::Renderers
{
	//defines a mesh
	struct StaticMesh
	{
		uint64 assetID = 0; //asset ID for the asset manager

		std::string declPath = ""; //the decl path

		std::vector<Smok::Mesh::Mesh> meshes; //the raw mesh data
		std::vector<uint32> megaMeshBufferIndexes; //the indexs into the mega mesh buffer
	};

	//manages assets
	struct AssetManager
	{
		VmaAllocator allocator;
		SMGraphics_Core_GPU* GPU;

		//texture descriptor stuff
		Smok::Texture::TextureBuffer textureBuffer;
		Smok::Mesh::Util::MegaMeshBuffer megaMeshBuffer; //the buffer of vertices

		BTD::IDStringHash IDRegistery; //the ID name registery

		std::unordered_map<uint64, Smok::Graphics::Pipeline::GraphicsShader> GShaderAssets; //the loaded shaders
		std::unordered_map<uint64, Smok::Graphics::Pipeline::GraphicsPipeline> GPipelineAssets; //the loaded graphics pipelines
		std::unordered_map<uint64, Smok::Texture::Texture> textureAssets; //the loaded textures
		std::unordered_map<uint64, Smok::Graphics::Util::Image::Sampler2D> samplerAssets; //the loaded samplers
		std::unordered_map<uint64, StaticMesh> staticMeshAssets; //the loaded meshes

		//inits the asset manager
		inline void Init(VmaAllocator _allocator,
			SMGraphics_Core_GPU* _GPU)
		{
			allocator = _allocator;
			GPU = _GPU;

			//start the ID registery to 1, so 0 can be the error
			IDRegistery.iDRegistery.nextID = 1;
		}

		//destroys all the assets
		inline void Destroy()
		{
			//wait for the GPU to finish
			vkDeviceWaitIdle(GPU->device);

			Mesh::Util::MegaMeshBuffer_DestroyBuffer(&megaMeshBuffer, allocator);
			

			//destroys the assets
			staticMeshAssets.clear();

			for (auto& pipeline : GPipelineAssets)
				Graphics::Pipeline::GraphicsPipeline_Destroy(&pipeline.second);
			GPipelineAssets.clear();

			for (auto& GShader : GShaderAssets)
				Graphics::Pipeline::GraphicsShader_Destroy(&GShader.second);
			GShaderAssets.clear();

			for (auto& sampler : samplerAssets)
				Graphics::Util::Image::Sampler2D_Destroy(&sampler.second, GPU);
			samplerAssets.clear();

			for (auto& texture : textureAssets)
			{
				vkDestroyImageView(GPU->device, texture.second.view, NULL);
				vmaDestroyImage(allocator, texture.second.image, texture.second.imageMemoy);
			}
			textureAssets.clear();

			IDRegistery.Clear();
		}

		//gets the asset's ID by name
		inline uint64 GetIDByName(const char* name, bool silenceErrors = false)
		{
			if (!IDRegistery.IsID(name))
			{
				if(!silenceErrors)
				BTD_LogError("Smok Renderer", "Asset Manager", "GetIDByName",
					std::string("\"" + std::string(name) + "\" is not a valid name for a registered asset!").c_str());
				return 0;
			}
			return IDRegistery.GetID(name);
		}

		//gets the asset's name by ID
		inline std::string GetNameByID(const uint64& ID) { return IDRegistery.GetStr(ID); }

		//gets a graphics shader
		inline Smok::Graphics::Pipeline::GraphicsShader* GetGraphicsShader(const uint64& ID, bool silenceErrors = false)
		{
			//checks if it matches the shaders
			for (auto& asset : GShaderAssets)
			{
				if (asset.first == ID)
					return &asset.second;
			}

			if(!silenceErrors)
				BTD_LogError("Smok Renderer", "Asset Manager", "GetGraphicsShader",
					std::string("ID is not valid for a Graphcis Shader!").c_str());

			return nullptr;
		}

		//gets a graphics shader
		inline Smok::Graphics::Pipeline::GraphicsShader* GetGraphicsShader(const char* name, bool silenceErrors = false)
		{
			Smok::Graphics::Pipeline::GraphicsShader* asset = GetGraphicsShader(GetIDByName(name, silenceErrors), silenceErrors);
			if(!asset && !silenceErrors)
				BTD_LogError("Smok Renderer", "Asset Manager", "GetGraphicsShader",
					std::string("\"" + std::string(name) + "\" is not a valid name for a Graphcis Shader!").c_str());

			return asset;
		}

		//gets a graphics pipeline
		inline Smok::Graphics::Pipeline::GraphicsPipeline* GetGraphicsPipeline(const uint64& ID, bool silenceErrors = false)
		{
			for (auto& asset : GPipelineAssets)
			{
				if (asset.first == ID)
					return &asset.second;
			}

			if (!silenceErrors)
				BTD_LogError("Smok Renderer", "Asset Manager", "GetGraphicsPipeline", "ID is not a valid for a Graphics Pipeline!");
			return nullptr;
		}

		//gets a graphics pipeline
		inline Smok::Graphics::Pipeline::GraphicsPipeline* GetGraphicsPipeline(const char* name, bool silenceErrors = false)
		{
			return GetGraphicsPipeline(GetIDByName(name, silenceErrors), silenceErrors);
		}

		//gets a texture 2D
		inline Smok::Texture::Texture* GetTexture(const uint64& ID, bool silenceErrors = false)
		{
			for (auto& asset : textureAssets)
			{
				if (asset.first == ID)
					return &asset.second;
			}

			if (!silenceErrors)
				BTD_LogError("Smok Renderer", "Asset Manager", "GetTexture", "ID is not a valid for a Texture!");
			return nullptr;
		}

		//gets a texture 2D
		inline Smok::Texture::Texture* GetTexture(const char* name, bool silenceErrors = false)
		{
			if (!IDRegistery.IsID(name))
			{
				if (!silenceErrors)
					BTD_LogError("Smok Renderer", "Asset Manager", "GetTexture",
						std::string("\"" + std::string(name) + "\" is not a valid name for a Texture!").c_str());
				return nullptr;
			}
			uint64 ID = IDRegistery.GetID(name);

			for (auto& asset : textureAssets)
			{
				if (asset.first == ID)
					return &asset.second;
			}

			if (!silenceErrors)
				BTD_LogError("Smok Renderer", "Asset Manager", "GetTexture",
					std::string("\"" + std::string(name) + "\" is not a valid name for a Texture!").c_str());
			return nullptr;
		}

		//gets a sampler 2D
		inline Smok::Graphics::Util::Image::Sampler2D* GetSampler2D(const char* name, bool silenceErrors = false)
		{
			if (!IDRegistery.IsID(name))
			{
				if (!silenceErrors)
					BTD_LogError("Smok Renderer", "Asset Manager", "GetSampler2D",
						std::string("\"" + std::string(name) + "\" is not a valid name for a Sampler2D!").c_str());
				return nullptr;
			}
			uint64 ID = IDRegistery.GetID(name);

			for (auto& asset : samplerAssets)
			{
				if (asset.first == ID)
					return &asset.second;
			}

			if (!silenceErrors)
				BTD_LogError("Smok Renderer", "Asset Manager", "GetSampler2D",
					std::string("\"" + std::string(name) + "\" is not a valid name for a Sampler2D!").c_str());
			return nullptr;
		}

		//gets a static mesh
		inline StaticMesh* GetStaticMesh(const uint64& ID, bool silenceErrors = false)
		{
			//checks if it matches the assets
			for (auto& asset : staticMeshAssets)
			{
				if (asset.first == ID)
					return &asset.second;
			}

			if (!silenceErrors)
				BTD_LogError("Smok Renderer", "Asset Manager", "GetStaticMesh",
					std::string("ID is not valid for a Static Mesh!").c_str());

			return nullptr;
		}

		//gets a static mesh
		inline StaticMesh* GetStaticMesh(const char* name, bool silenceErrors = false)
		{
			StaticMesh* asset = GetStaticMesh(GetIDByName(name, silenceErrors), silenceErrors);
			if (!asset && !silenceErrors)
				BTD_LogError("Smok Renderer", "Asset Manager", "GetStaticMesh",
					std::string("\"" + std::string(name) + "\" is not a valid name for a Static Mesh!").c_str());

			return asset;
		}

		//registers a graphics shader
		inline Smok::Graphics::Pipeline::GraphicsShader* RegisterGraphicsShader(const char* name,
			const char* declPath)
		{
			//checks if it already exists, if it does, return it
			Smok::Graphics::Pipeline::GraphicsShader* asset = GetGraphicsShader(name, true);
			if (asset)
				return asset;

			//creates the shader
			uint64 ID = 0;
			IDRegistery.GenerateNewID(name, ID);

			GShaderAssets[ID] = Smok::Graphics::Pipeline::GraphicsShader();
			asset = &GShaderAssets[ID];
			asset->declPath = declPath;
			asset->assetID = ID;
			return asset;
		}

		//registers a graphics pipeline
		inline Smok::Graphics::Pipeline::GraphicsPipeline* RegisterGraphicsPipeline(const char* name,
			const char* pipelineConfigDeclPath,
			const uint64& shaderAssetID)
		{
			//checks if it already exists, if it does, return it
			Smok::Graphics::Pipeline::GraphicsPipeline* asset = GetGraphicsPipeline(name, true);
			if (asset)
				return asset;

			//creates the asset
			uint64 ID = 0;
			IDRegistery.GenerateNewID(name, ID);

			GPipelineAssets[ID] = Smok::Graphics::Pipeline::GraphicsPipeline();
			asset = &GPipelineAssets[ID];

			asset->assetID = ID;
			//asset->declConfigDeclPath = pipelineConfigDeclPath;

			asset->graphicsShaderAssetID = shaderAssetID;
			
			return asset;
		}

		//registers a texture 2D
		inline Smok::Texture::Texture* RegisterTexture(const char* name,
			const char* declPath)
		{
			//checks if it already exists, if it does, return it
			Smok::Texture::Texture* asset = GetTexture(name, true);
			if (asset)
				return asset;

			//creates the asset
			uint64 ID = 0;
			IDRegistery.GenerateNewID(name, ID);

			textureAssets[ID] = Smok::Texture::Texture();
			asset = &textureAssets[ID];
			asset->assetID = ID;
			asset->declPath = declPath;

			return asset;
		}

		//registers a sampler 2D
		inline Smok::Graphics::Util::Image::Sampler2D* RegisterSampler2D(const char* name,
			const char* declPath)
		{
			//checks if it already exists, if it does, return it
			Smok::Graphics::Util::Image::Sampler2D* asset = GetSampler2D(name, true);
			if (asset)
				return asset;

			//creates the asset
			uint64 ID = 0;
			IDRegistery.GenerateNewID(name, ID);

			samplerAssets[ID] = Smok::Graphics::Util::Image::Sampler2D();
			asset = &samplerAssets[ID];
			asset->declPath = declPath;
			asset->assetID = ID;

			return asset;
		}

		//registers a static mesh
		inline StaticMesh* RegisterStaticMesh(const char* name,
			const char* declPath)
		{
			//checks if it already exists, if it does, return it
			StaticMesh* asset = GetStaticMesh(name, true);
			if (asset)
				return asset;

			//creates the asset
			uint64 ID = 0;
			IDRegistery.GenerateNewID(name, ID);

			staticMeshAssets[ID] = StaticMesh();
			asset = &staticMeshAssets[ID];
			asset->assetID = ID;
			asset->declPath = declPath;

			return asset;
		}
	
		//creates a graphics shader
		inline Smok::Graphics::Pipeline::GraphicsShader* CreateGraphicsShader(const uint64& ID)
		{
			Smok::Graphics::Pipeline::GraphicsShader* asset = GetGraphicsShader(ID, true);
			if (asset->fMod != VK_NULL_HANDLE)
				return asset;

			//loads the YAML data
			std::string assetName = "", vPath = "", fPath = "";
			Smok::Graphics::Pipeline::GraphicsShader_LoadDeclFile(asset->declPath, assetName, vPath, fPath);

			return (Graphics::Pipeline::GraphicsShader_Create(asset, GPU,
				vPath.c_str(), fPath.c_str()) == true ? asset : nullptr);
		}

		//creates a graphics shader
		inline Smok::Graphics::Pipeline::GraphicsShader* CreateGraphicsShader(const char* name)
		{
			return CreateGraphicsShader(GetIDByName(name));
		}

		//creates a graphics pipeline
		inline Graphics::Pipeline::GraphicsPipeline* CreateGraphicsPipeline(const uint64& GPipelineID,
			VkPipelineLayout& pipelineLayout, VkRenderPass& renderpass)
		{
			Graphics::Pipeline::GraphicsPipeline* asset = GetGraphicsPipeline(GPipelineID, true);// = &GPipelineAssets.at(GPipelineID);
			if (asset->pipeline != VK_NULL_HANDLE)
				return asset;

			Graphics::Pipeline::GraphicsPipeline_Create(asset, GPU->device,
				pipelineLayout, CreateGraphicsShader(asset->graphicsShaderAssetID), renderpass,
				Smok::Mesh::Vertex::VertexLayout().GenBindDesc(), Smok::Mesh::Vertex::VertexLayout().GenAttDesc());

			return asset;
		}

		//creates a graphics pipeline
		inline Graphics::Pipeline::GraphicsPipeline* CreateGraphicsPipeline(const char* name,
			VkPipelineLayout& pipelineLayout, VkRenderPass& renderpass)
		{
			return CreateGraphicsPipeline(GetIDByName(name), pipelineLayout, renderpass);
		}

		//creates a texture
		inline Smok::Texture::Texture* CreateTexture(const uint64& ID, SMGraphics_Pool_CommandPool* commandPool)
		{
			Smok::Texture::Texture* asset = &textureAssets[ID];
			if (asset->image != VK_NULL_HANDLE)
				return asset;

			std::string assetName = "", binaryPath = "";
			if (!Smok::Texture::Texture_LoadDecl(asset->declPath, assetName, binaryPath))
			{
				BTD_LogError("Smok Renderer", "Asset Manager",
					"CreateTexture2D",
					std::string("Failed to load a texture decl file at \"" + binaryPath + "\"").c_str());
				return asset;
			}

			if (!Smok::Texture::Texture_Create(asset, binaryPath, allocator, GPU, commandPool))
			{
				BTD_LogError("Smok Renderer", "Asset Manager",
					"CreateTexture2D",
					std::string("Failed to create a texture from a decl file at \"" + binaryPath + "\"").c_str());
			}

			return asset;
		}

		//creates a texture
		inline Smok::Texture::Texture* CreateTexture(const char* name, SMGraphics_Pool_CommandPool* commandPool)
		{
			return CreateTexture(GetIDByName(name), commandPool);
		}

		//creates a sampler 2D
		inline Smok::Graphics::Util::Image::Sampler2D* CreateSampler2D(const uint64& ID)
		{
			Smok::Graphics::Util::Image::Sampler2D* asset = &samplerAssets[ID];
			if (asset->sampler != VK_NULL_HANDLE)
				return asset;

			Smok::Graphics::Util::Image::Sampler2D_DeclData declData;
			Smok::Graphics::Util::Image::Sampler2D_Decl_LoadFile(asset->declPath, declData);
			Smok::Graphics::Util::Image::Sampler2D_Create(asset, GPU, declData);

			return asset;
		}

		//creates a sampler 2D
		inline Smok::Graphics::Util::Image::Sampler2D* CreateSampler2D(const char* name)
		{
			return CreateSampler2D(GetIDByName(name));
		}

		//creates a static mesh
		inline StaticMesh* CreateStaticMesh(const uint64& staticMeshID)
		{
			StaticMesh* asset = GetStaticMesh(staticMeshID, true);

			//if the mesh asset is already loaded
			if (asset->meshes.size() > 0)
				return asset;

			//loads mesh
			Smok::Mesh::MeshDeclData declData;
			Smok::Mesh::Mesh_LoadMeshDataFromFile(asset->declPath, declData);

			//pushes the meshes into the mega mesh buffer
			const size_t meshCount = declData.meshCount;
			asset->megaMeshBufferIndexes.resize(meshCount);
			asset->meshes = declData.meshes;

			for (uint32 i = 0; i < meshCount; ++i)
				Mesh::Util::MegaMeshBuffer_AddMesh(&megaMeshBuffer, asset->meshes[i],
					asset->megaMeshBufferIndexes[i]);

			return asset;
		}

		//creates a static mesh
		inline StaticMesh* CreateStaticMesh(const char* staticMeshName)
		{
			return CreateStaticMesh(GetIDByName(staticMeshName));
		}

		//destroy a graphics shader
		  
		//destroy a graphics pipeline

		//destroy a texture
		 
		//destroy a sampler 2D
		 
		//destroy a static mesh

		//remakes all the graphics pipelines
		inline void RemakeGraphicsPipelines(VkRenderPass& renderpass)
		{
			for (auto& asset : GPipelineAssets)
			{
				if (asset.second.pipeline != VK_NULL_HANDLE)
					Graphics::Pipeline::GraphicsPipeline_Recreate(&asset.second, renderpass);
			}
		}
	};
}