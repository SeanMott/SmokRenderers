#pragma once

//defines a render manager for managing a frame and it's graph

#include <SmokWindow/Desktop/DesktopWindow.h>

namespace Smok::Renderers
{
	//defines a frame, holding the image index and render targets
	struct Frame
	{
		bool isValid = false; //is the frame valid
		uint32 imageIndex = 0; //the index into the image array in the swapchain
		uint32 frameIndex = 0; //the current frame of the swapchain
		uint32 currentFrame = 0; //the current frame of the game
		BTD_Math_U32Vec2 frameSize; //the size of the frame
		VkFramebuffer framebuffer = VK_NULL_HANDLE; //swapchain frame buffers
	};

	//defines a render manager
	struct RenderManager
	{
		VkSemaphore imageAvailableSemaphores[2];
		VkSemaphore renderFinishedSemaphores[2];
		VkFence inFlightFences[2];
		VkFence imagesInFlight[3] = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE };

		const uint8 maxFramesInFlight = 2;
		size_t currentFrame = 0;

		VkQueue graphicsQueue = VK_NULL_HANDLE, presentQueue = VK_NULL_HANDLE;
		VkDevice device = VK_NULL_HANDLE;
	};

	//initalizes the render manager
	inline bool InitRenderManager(RenderManager* renderManager, SMGraphics_Core_GPU* GPU)
	{
		//creates rendering sync objects
		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < renderManager->maxFramesInFlight; i++)
		{
			if (vkCreateSemaphore(GPU->device, &semaphoreInfo, nullptr, &renderManager->imageAvailableSemaphores[i]) !=
				VK_SUCCESS ||
				vkCreateSemaphore(GPU->device, &semaphoreInfo, nullptr, &renderManager->renderFinishedSemaphores[i]) !=
				VK_SUCCESS ||
				vkCreateFence(GPU->device, &fenceInfo, nullptr, &renderManager->inFlightFences[i]) != VK_SUCCESS)
			{
				BTD::Logger::LogError("Smok Renderers", "Render Manager", "InitRenderManager", "Failed to create synchronization objects!");
				return false;
			}
		}

		return true;
	}

	//shutsdown the render manager
	inline void ShutdownRenderManager(RenderManager* renderManager, SMGraphics_Core_GPU* GPU)
	{
		for (size_t i = 0; i < 2; i++) {
			vkDestroySemaphore(GPU->device, renderManager->renderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(GPU->device, renderManager->imageAvailableSemaphores[i], nullptr);
			vkDestroyFence(GPU->device, renderManager->inFlightFences[i], nullptr);
		}
		renderManager->imagesInFlight[0] = VK_NULL_HANDLE; renderManager->imagesInFlight[1] = VK_NULL_HANDLE; renderManager->imagesInFlight[2] = VK_NULL_HANDLE;
	}

	//gets the next frame
	inline bool NextFrame(RenderManager* renderManager, SMGraphics_Core_GPU* GPU,
		SMWindow_Desktop_Swapchain* swapchain,
		Frame& frame)
	{
		//if not valid
		/*if (!display || !display->isRunning || display->swapchain.swapchain == VK_NULL_HANDLE || display->displayWasCleanedUpEarly)
			return Frame();*/

		vkWaitForFences(GPU->device, 1, &renderManager->inFlightFences[renderManager->currentFrame], VK_TRUE, UINT64_MAX); //waits for fence

		//gets next frame

		VkResult result = vkAcquireNextImageKHR(GPU->device, swapchain->swapchain, UINT64_MAX,
			renderManager->imageAvailableSemaphores[renderManager->currentFrame],  // must be a not signaled semaphore
			VK_NULL_HANDLE,
			&frame.imageIndex);

		//if not valid
		if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR)
			return false;

		//if is valid
		frame.isValid = true;
		frame.framebuffer = swapchain->framebuffers[frame.imageIndex];
		frame.currentFrame;
		frame.frameSize = Smok_Util_Typepun(swapchain->extents, BTD_Math_U32Vec2);

		return true;
	}

	//submits a frame for rendering
	inline bool SubmitFrame(RenderManager* renderManager, SMGraphics_Core_GPU* GPU, VkSwapchainKHR* swapchain, Frame& frame,
		VkCommandBuffer* comBuffers, const uint8 comBufferCount = 1)
	{
		if (frame.framebuffer == VK_NULL_HANDLE)
			return false;

		//images in flight check
		if (renderManager->imagesInFlight[frame.imageIndex] != VK_NULL_HANDLE) {
			vkWaitForFences(GPU->device, 1, &renderManager->imagesInFlight[frame.imageIndex], VK_TRUE, UINT64_MAX);
		}
		renderManager->imagesInFlight[frame.imageIndex] = renderManager->inFlightFences[renderManager->currentFrame];

		//sumbits command buffers
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { renderManager->imageAvailableSemaphores[renderManager->currentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = comBufferCount;
		submitInfo.pCommandBuffers = comBuffers;

		VkSemaphore signalSemaphores[] = { renderManager->renderFinishedSemaphores[renderManager->currentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		vkResetFences(GPU->device, 1, &renderManager->inFlightFences[renderManager->currentFrame]);
		if (vkQueueSubmit(GPU->graphicsQueue, 1, &submitInfo, renderManager->inFlightFences[renderManager->currentFrame]) !=
			VK_SUCCESS) {
			BTD::Logger::LogError("Smok Renderers", "Render Manager", "SubmitFrame", "Failed to submit draw command buffer!");
			return false;
		}

		vkDeviceWaitIdle(GPU->device); //wait for it to be done

		//submits swapchains
		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapchain;

		presentInfo.pImageIndices = &frame.imageIndex;

		vkQueuePresentKHR(GPU->presentQueue, &presentInfo);
		vkDeviceWaitIdle(GPU->device);

		renderManager->currentFrame = (renderManager->currentFrame + 1) % renderManager->maxFramesInFlight;

		return true;
	}

}