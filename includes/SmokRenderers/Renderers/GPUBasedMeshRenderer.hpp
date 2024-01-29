#pragma once

//defines a GPU based mesh renderer

#include <BTDSTD/Math/RenderMath.hpp>
#include <BTDSTD_C/Math/Vectors.h>

#include <SmokRenderers/RenderManager.hpp>

#include <SmokMesh/Mesh.hpp>
#include <SmokTexture/Texture.hpp>
#include <SmokTexture/TextureBuffer.hpp>

#include <SmokGraphics/Pools/Descriptor.hpp>

#include <SmokWindow/Desktop/DesktopWindow.h>

#include <BTDSTD/Maps/IDHash.hpp>
#include <BTDSTD/Math/RenderMath.hpp>

#include <SmokRenderers/AssetManager.hpp>

namespace Smok::Renderers::GPUBased::MeshRenderer
{
	//defines a buffer for the camera buffer
#define SMOK_RENDERER_CAMERA_BUFFER_ARRAY_LENGTH 4
	struct CameraBuffer
	{
		glm::mat4 P[SMOK_RENDERER_CAMERA_BUFFER_ARRAY_LENGTH]; //the projection matrix
		glm::mat4 V[SMOK_RENDERER_CAMERA_BUFFER_ARRAY_LENGTH]; //the view matrix
		glm::mat4 PV[SMOK_RENDERER_CAMERA_BUFFER_ARRAY_LENGTH]; //the projection * view matrix
	};

	//defines a object in the object GPU buffer
	struct ObjectBuffer_Object
	{
		glm::mat4 model = glm::mat4(1.0f); //the moxel matrix
		glm::vec4 metadata; //metadata
		/*
		x = camera index
		y = texture index
		z = mesh index
		w = not used
		*/
	};

	

	//defines a buffer for all the mesh data

	//defines a buffer for all the indirect render commands

	//defines a indirect render command
	struct RenderCommand
	{
		uint64 meshIndex = 0, //the mesh index
			objIndex = 0; //the object index
	};

	//defines a render batch
	struct RenderBatch
	{
		uint64 pipelineID; //the pipeline to use

		std::vector<ObjectBuffer_Object> objs; //the objects
		std::vector<RenderCommand> commands; //the render commands
	};

	//defines a sorted object data for a batch
	struct ObjectBatch_Object
	{
		uint64 pipelineID = 0; //the graphics pipeline to use
		
		std::vector<uint32> megaMeshBufferIndexs; //the indexes into the mega mesh buffer to use
		
		//the textures

		ObjectBuffer_Object obj; //the object data
	};

	//defines a GPU Based Mesh Renderer
	class GPUMeshRenderer
	{
		//vars
	private:

		Graphics::Descriptor::DescriptorPool descriptorPool;
		Graphics::Pipeline::GraphicsPipelineLayout graphicsPipelineLayout;

		//camera descriptor stuff
		Graphics::Descriptor::DescriptorSetLayout cameraBufferDescriptorSetLayout;
		Graphics::Descriptor::DescriptorSet cameraBufferDescSet;

		//object descriptor stuff
		Graphics::Descriptor::DescriptorSetLayout objectBufferDescriptorSetLayout;
		Graphics::Descriptor::DescriptorSet objectBufferDescSet;
		std::vector<uint32> lastFrameObjectCount; //stores the last frame of object size, so we can resize it

		//texture descriptor stuff
		Graphics::Descriptor::DescriptorSetLayout textureDescriptorSetLayout;
		Graphics::Descriptor::DescriptorSet textureDescSet;

		SMGraphics_Core_GPU* GPU;
		SMWindow_Desktop_Swapchain* swapchain;
		VmaAllocator allocator;
		SMGraphics_Pool_CommandPool* commandPool;
		AssetManager* assetManager;

		//methods
	public:

		//inits the renderer
		inline bool Init(SMGraphics_Core_GPU* _GPU, VmaAllocator& _allocator,
			 SMWindow_Desktop_Swapchain* _swapchain, SMGraphics_Pool_CommandPool* _commandPool,
			AssetManager* _assetManager,
			const uint64& blankTextureID, const uint64& blankSampler2DID)
		{
			GPU = _GPU; allocator = _allocator; swapchain = _swapchain;
			commandPool = _commandPool;

			assetManager = _assetManager;

			//loads the default texture and sampler
			Smok::Texture::Texture* blankTexture = assetManager->CreateTexture(blankTextureID, commandPool);
			Smok::Graphics::Util::Image::Sampler2D* blankSampler = assetManager->CreateSampler2D(blankSampler2DID);

			//creates a descriptor pool
			Smok::Graphics::Descriptor::DescriptorSetPoolCreateInfo descriptorPoolCreateInfo;
			descriptorPoolCreateInfo.maxSetCount = swapchain->framesInFlight * 3;
			descriptorPoolCreateInfo.uniformBufferPoolCount = swapchain->framesInFlight;
			descriptorPoolCreateInfo.uniformStorageBufferPoolCount = swapchain->framesInFlight;
			descriptorPoolCreateInfo.uniformSampler2DArrayPoolCount = swapchain->framesInFlight;

			Smok::Graphics::Descriptor::DescriptorPool_Create(&descriptorPool, descriptorPoolCreateInfo, GPU);

			//--------------CAMERA BUFFER DESC----------------//

			Smok::Graphics::Util::Uniform::UniformBuffer UniformBuffer_CameraBuffer;
			UniformBuffer_CameraBuffer.name = "CameraBuffer";
			UniformBuffer_CameraBuffer.binding = 0;
			UniformBuffer_CameraBuffer.shaderAccessStages = VK_SHADER_STAGE_VERTEX_BIT;
			UniformBuffer_CameraBuffer.structMemSize = sizeof(CameraBuffer);

			//defines a descriptor set layout
			Smok::Graphics::Descriptor::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
			descriptorSetLayoutCreateInfo.uniforms.uniformBuffers.emplace_back(UniformBuffer_CameraBuffer);

			Smok::Graphics::Descriptor::DescriptorSetLayout_Create(&cameraBufferDescriptorSetLayout, descriptorSetLayoutCreateInfo, GPU);

			//creates a descriptor set
			Smok::Graphics::Descriptor::DescriptorSet_Create(&cameraBufferDescSet, &cameraBufferDescriptorSetLayout, &descriptorPool,
				allocator, GPU, swapchain->framesInFlight);

			//--------------OBJECT BUFFER DESC----------------//

			Smok::Graphics::Util::Uniform::UniformStorageBuffer UniformStorgaeBuffer_ObjectBuffer;
			UniformStorgaeBuffer_ObjectBuffer.name = "ObjectBuffer";
			UniformStorgaeBuffer_ObjectBuffer.binding = 0;
			UniformStorgaeBuffer_ObjectBuffer.shaderAccessStages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			UniformStorgaeBuffer_ObjectBuffer.structMemSize = sizeof(ObjectBatch_Object);

			//defines a descriptor set layout
			descriptorSetLayoutCreateInfo.uniforms.Clear();
			descriptorSetLayoutCreateInfo.uniforms.uniformStorageBuffers.emplace_back(UniformStorgaeBuffer_ObjectBuffer);

			Smok::Graphics::Descriptor::DescriptorSetLayout_Create(&objectBufferDescriptorSetLayout, descriptorSetLayoutCreateInfo, GPU);

			//creates a descriptor set
			Smok::Graphics::Descriptor::DescriptorSet_Create(&objectBufferDescSet, &objectBufferDescriptorSetLayout, &descriptorPool,
				allocator, GPU, swapchain->framesInFlight);

			lastFrameObjectCount.resize(swapchain->framesInFlight, 0);

			//--------------TEXTURE BUFFER DESC----------------//
			descriptorSetLayoutCreateInfo.uniforms.Clear();

			Smok::Graphics::Util::Uniform::Sampler2DArray UniformSampler2DArray_Textures;
			UniformSampler2DArray_Textures.name = "Textures";
			UniformSampler2DArray_Textures.binding = 0;
			UniformSampler2DArray_Textures.shaderAccessStages = VK_SHADER_STAGE_FRAGMENT_BIT;
			UniformSampler2DArray_Textures.arrayLength = 15;
			UniformSampler2DArray_Textures.blankView = blankTexture->view;
			UniformSampler2DArray_Textures.blankSampler = blankSampler->sampler;

			//defines a descriptor set layout
		
			descriptorSetLayoutCreateInfo.uniforms.sampler2DArrays.emplace_back(UniformSampler2DArray_Textures);

			Smok::Graphics::Descriptor::DescriptorSetLayout_Create(&textureDescriptorSetLayout, descriptorSetLayoutCreateInfo, GPU);

			//creates a descriptor set
			Smok::Graphics::Descriptor::DescriptorSet_Create(&textureDescSet, &textureDescriptorSetLayout, &descriptorPool,
				allocator, GPU, swapchain->framesInFlight);

			//create a graphics pipeline layout
			Smok::Graphics::Pipeline::GraphicsPipelineLayoutCreateInfo graphicsPipelineLayoutCreateInfo;
			graphicsPipelineLayoutCreateInfo.descriptorLayouts = { cameraBufferDescriptorSetLayout.descriptorSetLayout,
			objectBufferDescriptorSetLayout.descriptorSetLayout, textureDescriptorSetLayout.descriptorSetLayout };

			Smok::Graphics::Pipeline::GraphicsPipelineLayout_Create(&graphicsPipelineLayout, GPU,
				graphicsPipelineLayoutCreateInfo);

			return true;
		}

		//shutsdown the renderer
		inline void Shutdown()
		{
			//wait for the GPU to finish
			vkDeviceWaitIdle(GPU->device);

			Smok::Graphics::Pipeline::GraphicsPipelineLayout_Destroy(&graphicsPipelineLayout);

			Smok::Graphics::Descriptor::DescriptorSet_Destroy(&textureDescSet, &descriptorPool, allocator, GPU);
			Smok::Graphics::Descriptor::DescriptorSetLayout_Destroy(&textureDescriptorSetLayout, GPU);
			
			Smok::Graphics::Descriptor::DescriptorSet_Destroy(&objectBufferDescSet, &descriptorPool, allocator, GPU);
			Smok::Graphics::Descriptor::DescriptorSetLayout_Destroy(&objectBufferDescriptorSetLayout, GPU);

			Smok::Graphics::Descriptor::DescriptorSet_Destroy(&cameraBufferDescSet, &descriptorPool, allocator, GPU);
			Smok::Graphics::Descriptor::DescriptorSetLayout_Destroy(&cameraBufferDescriptorSetLayout, GPU);

			Smok::Graphics::Descriptor::DescriptorPool_Destroy(&descriptorPool, GPU);
		}

		//gets the texture descriptor set
		inline Graphics::Descriptor::DescriptorSet* GetTextureDescriptorSet() { return &textureDescSet; }

		//purges all objects in the object buffer
		inline void PurgeAllObjects(const uint32& frameIndex)
		{
			VkWriteDescriptorSet descriptorWrite = {}; VkDescriptorBufferInfo bufferInfo = {};

			bool state = false;
			Graphics::Descriptor::DescriptorSet_UniformStorageBuffer_RecreateBuffer(objectBufferDescSet.descriptorSets[frameIndex],
				&objectBufferDescSet.uniformStorageBuffers["ObjectBuffer"].buffers[frameIndex],
				state,
				sizeof(ObjectBuffer_Object),
				&descriptorWrite, &bufferInfo, allocator);

			//copies over if it's safe memory copy, just in case the reallocation threw it somewhere new
			objectBufferDescSet.uniformStorageBuffers["ObjectBuffer"].isSafeCopy[frameIndex] = state;

			//updates bindings
			vkUpdateDescriptorSets(GPU->device, 1, &descriptorWrite, 0, nullptr);

			lastFrameObjectCount[frameIndex] = 0;
		}

		//adds a object for rendering into the batch || if all meshes should be rendered, a empty list of meshIndexes can be passed in
		inline void AddObject(BTD::Math::Transform* transform,
			const uint64& staticMeshID,
			const uint64& graphicsShaderID,
			const uint64& graphicsPipelineID,
			const uint64& textureID,
			const uint64& samplerID,
			//const std::vector<uint32>& _meshIndexes,
			std::vector<ObjectBatch_Object>& objects)
		{
			//loads/gets the assets
			StaticMesh* staticMesh = assetManager->CreateStaticMesh(staticMeshID);
			if (!staticMesh)
				return;

			Smok::Graphics::Pipeline::GraphicsShader* shader = assetManager->CreateGraphicsShader(graphicsShaderID);
			Smok::Graphics::Pipeline::GraphicsPipeline* pipeline = assetManager->CreateGraphicsPipeline(graphicsPipelineID,
				graphicsPipelineLayout.pipelineLayout, swapchain->renderpass);
			Smok::Texture::Texture* texture = assetManager->CreateTexture(textureID, commandPool);
			Smok::Graphics::Util::Image::Sampler2D* sampler = assetManager->CreateSampler2D(samplerID);

			//calculates each object
			ObjectBatch_Object* obj = &objects.emplace_back(ObjectBatch_Object());

			//sets the pipeline to use
			obj->pipelineID = pipeline->assetID;

			//matches the sought after mesh indices and the indexes into the mega mesh buffer

			//if no mesh indexes were specificed, we get them all
			obj->megaMeshBufferIndexs = staticMesh->megaMeshBufferIndexes;

			//if they were specificed, we get only the specific ones

			//model matrix
			obj->obj.model = transform->CalculateModelMatrix_Force();

			obj->obj.metadata.x = 0; //camera index

			//appends the texture to the buffer, and gets it's position for the object to use
			obj->obj.metadata.y = assetManager->textureBuffer.AddTexture(texture->view, sampler->sampler);
		}

		//calculates the indirect commands and mesh data
		inline void CalculateCommandData(const std::vector<ObjectBatch_Object>& objects,
			std::vector<RenderBatch>& renderBatch, std::vector<ObjectBuffer_Object>& objectBufferObjects)
		{
			renderBatch.clear(); renderBatch.reserve(2);
			objectBufferObjects.clear(); objectBufferObjects.reserve(256);
			
			//goes through the objects
			RenderBatch* batch = &renderBatch.emplace_back(RenderBatch());
			batch->pipelineID = objects[0].pipelineID;
			uint32 objIndex = 0;

			//makes a new batch

			for (uint32 i = 0; i < objects.size(); ++i)
			{
				for (uint32 m = 0; m < objects[i].megaMeshBufferIndexs.size(); ++m)
				{
					//add object
					objectBufferObjects.emplace_back(objects[i].obj);

					//add command
					RenderCommand* command = &batch->commands.emplace_back(RenderCommand());
					command->meshIndex = objects[i].megaMeshBufferIndexs[m];
					command->objIndex = i;
					objIndex++;
				}
			}

			//generate final mega mesh buffer
			Mesh::Util::MegaMeshBuffer_CreateBuffer(&assetManager->megaMeshBuffer, allocator, GPU, commandPool);
		}

		//registers a camera

		//updates the camera
		inline void UpdateCamera(const CameraBuffer* camData)
		{
			//copies data to GPU on all frames of the buffer
			Smok::Graphics::Descriptor::DescriptorSet_UniformBuffer_UploadDataToGPU_AllBuffers(
				&cameraBufferDescSet, "CameraBuffer",
				allocator, GPU, (void*)camData, commandPool);
		}

		//renders
		inline void Render(VkCommandBuffer& comBuffer, Frame& frame,
			const std::vector<RenderBatch>& renderBatch,
			const std::vector<ObjectBuffer_Object>& objectBufferObjects)
		{
			const size_t objCount = objectBufferObjects.size();

			//if nothing to render, leave
			if (!objCount)
				return;

			//if the object count is larger then it was last frame, we resize the buffer
			if (objCount > lastFrameObjectCount[frame.frameIndex])
			{
				VkWriteDescriptorSet descriptorWrite = {}; VkDescriptorBufferInfo bufferInfo = {};

				bool state = false;
				Graphics::Descriptor::DescriptorSet_UniformStorageBuffer_RecreateBuffer(objectBufferDescSet.descriptorSets[frame.frameIndex],
					&objectBufferDescSet.uniformStorageBuffers["ObjectBuffer"].buffers[frame.frameIndex],
					state,
					sizeof(ObjectBuffer_Object) * objCount,
					&descriptorWrite, &bufferInfo, allocator);
			
				//copies over if it's safe memory copy, just in case the reallocation threw it somewhere new
				objectBufferDescSet.uniformStorageBuffers["ObjectBuffer"].isSafeCopy[frame.frameIndex] = state;
			
				//updates bindings
				vkUpdateDescriptorSets(GPU->device, 1, &descriptorWrite, 0, nullptr);

				lastFrameObjectCount[frame.frameIndex] = objCount;
			}

			//copies object data
			memcpy(objectBufferDescSet.uniformStorageBuffers["ObjectBuffer"].buffers[frame.frameIndex].allocationInfo.pMappedData,
				objectBufferObjects.data(), objectBufferDescSet.uniformStorageBuffers["ObjectBuffer"].buffers[frame.frameIndex].size);// sizeof(ObjectBatch_Object)* objCount);

			//copies texture data
			if (assetManager->textureBuffer.sizeHasChanged)
			{
				Graphics::Descriptor::DescriptorSet_UniformSampler2DArray_UploadDataToGPU_AllBuffers(&textureDescSet, "Textures",
					GPU, assetManager->textureBuffer.textureViews.data(), assetManager->textureBuffer.textureSamplers.data(), assetManager->textureBuffer.textureSamplers.size());
				assetManager->textureBuffer.sizeHasChanged = false;
			}

			//goes through the batches
			for (uint32 b = 0; b < renderBatch.size(); ++b)
			{
				//bind pipeline
				Smok::Graphics::Pipeline::GraphicsPipeline_Bind(
					assetManager->GetGraphicsPipeline(assetManager->GetNameByID(renderBatch[b].pipelineID).c_str()), comBuffer);

				//sets viewport and scissor
				Smok::Graphics::Pipeline::GraphicsPipeline_SetViewportAndScissor(
					comBuffer,
					frame.frameSize, { 0, 0 });

				//binds the descriptor sets
				VkDescriptorSet sets[3] = { cameraBufferDescSet.descriptorSets[frame.currentFrame],
				objectBufferDescSet.descriptorSets[frame.currentFrame],
				textureDescSet.descriptorSets[frame.currentFrame] };
				vkCmdBindDescriptorSets(comBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
					graphicsPipelineLayout.pipelineLayout, 0, 3,
					sets, 0, nullptr);

				//if there is data to draw
				if (assetManager->megaMeshBuffer.vertexBuffer.vertexCount > 0)
				{
					//binds buffer
					Smok::Mesh::Util::MegaMeshBuffer_Bind(&assetManager->megaMeshBuffer, comBuffer);

					//draws
					for (uint32 i = 0; i < renderBatch[b].commands.size(); ++i)
					{
						Smok::Mesh::Util::MegaMeshBuffer_Draw(&assetManager->megaMeshBuffer,
							comBuffer, renderBatch[b].commands[i].meshIndex,
							renderBatch[b].commands[i].objIndex, objCount);
					}
				}
			}
		}
	};
}