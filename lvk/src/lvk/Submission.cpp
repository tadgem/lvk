#include "lvk/Submission.h"
#include "lvk/Init.h"
#include "lvk/Macros.h"
#include "lvk/Commands.h"
#include "ImGui/imgui.h"
#include "spdlog/spdlog.h"
#include "ImGui/imgui_impl_vulkan.h"

void lvk::submission::SubmitFrame(VkState& vk)
{
  if(vk.m_RunComputeCommands)
  {
    // Compute
    vkWaitForFences(vk.m_LogicalDevice, 1, &vk.m_ComputeInFlightFences[vk.m_CurrentFrameIndex], VK_TRUE,  UINT64_MAX);
    vkResetFences(vk.m_LogicalDevice, 1, &vk.m_ComputeInFlightFences[vk.m_CurrentFrameIndex]);

    VkSubmitInfo computeSubmitInfo {};
    computeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    computeSubmitInfo.commandBufferCount = 1;
    computeSubmitInfo.pCommandBuffers = &vk.m_ComputeCommandBuffers[vk.m_CurrentFrameIndex];
    computeSubmitInfo.signalSemaphoreCount = 1;
    computeSubmitInfo.pSignalSemaphores = &vk.m_ComputeFinishedSemaphores[vk.m_CurrentFrameIndex];

    VK_CHECK(vkQueueSubmit(vk.m_ComputeQueue, 1, &computeSubmitInfo, vk.m_ComputeInFlightFences[vk.m_CurrentFrameIndex]));
  }
  // Graphics
  vkWaitForFences(vk.m_LogicalDevice, 1, &vk.m_FrameInFlightFences[vk.m_CurrentFrameIndex], VK_TRUE, UINT64_MAX);

  uint32_t imageIndex;
  VkResult result = vkAcquireNextImageKHR(vk.m_LogicalDevice, vk.m_SwapChain,
                                          UINT64_MAX, vk.m_ImageAvailableSemaphores[vk.m_CurrentFrameIndex], VK_NULL_HANDLE, &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    init::RecreateSwapChain(vk);
    return;
  }
  else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    spdlog::error("VulkanAPI : Failed to acquire swap chain image!");
    return;
  }

  vkResetFences(vk.m_LogicalDevice, 1, &vk.m_FrameInFlightFences[vk.m_CurrentFrameIndex]);

  vk.m_ImagesInFlightFences[imageIndex] = vk.m_FrameInFlightFences[vk.m_CurrentFrameIndex];

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  Vector<VkSemaphore> waitSemaphores {};
  Vector<VkPipelineStageFlags> waitStages;

  waitSemaphores.push_back(vk.m_ImageAvailableSemaphores[vk.m_CurrentFrameIndex]);
  waitStages.push_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
  if(vk.m_RunComputeCommands)
  {
    waitSemaphores.push_back(vk.m_ComputeFinishedSemaphores[vk.m_CurrentFrameIndex]);
    waitStages.push_back(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
  }

  VkSemaphore signalSemaphores[]       = { vk.m_RenderFinishedSemaphores[vk.m_CurrentFrameIndex]};
  submitInfo.waitSemaphoreCount       = waitSemaphores.size();
  submitInfo.pWaitSemaphores          = waitSemaphores.data();
  submitInfo.pWaitDstStageMask        = waitStages.data();
  submitInfo.commandBufferCount       = 1;
  submitInfo.pCommandBuffers          = &vk.m_GraphicsCommandBuffers[imageIndex];
  submitInfo.signalSemaphoreCount     = 1;
  submitInfo.pSignalSemaphores        = signalSemaphores;

  vkResetFences(vk.m_LogicalDevice, 1, &vk.m_FrameInFlightFences[vk.m_CurrentFrameIndex]);

  if (vkQueueSubmit(vk.m_GraphicsQueue, 1, &submitInfo, vk.m_FrameInFlightFences[vk.m_CurrentFrameIndex]) != VK_SUCCESS)
  {
    spdlog::error("VulkanAPI : Failed to submit draw command buffer!");
  }

  if (vk.m_UseImGui)
  {
    RenderImGui(vk);
  }

  VkPresentInfoKHR presentInfo{};
  presentInfo.sType               = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount  = 1;
  presentInfo.pWaitSemaphores     = signalSemaphores;
  presentInfo.swapchainCount      = 1;
  VkSwapchainKHR swapchains[]     = { vk.m_SwapChain };
  presentInfo.pSwapchains         = swapchains;
  presentInfo.pImageIndices       = &imageIndex;
  presentInfo.pResults            = nullptr;

  result = vkQueuePresentKHR(vk.m_GraphicsQueue, &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    init::RecreateSwapChain(vk);
    return;
  }
  else if (result != VK_SUCCESS) {
    spdlog::error("VulkanAPI : Error presenting swapchain image");
  }

  vk.m_CurrentFrameIndex = (vk.m_CurrentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}


void lvk::submission::RenderImGui(VkState& vk)
{
  ImGui::Render();
  VkCommandBuffer imguiCommandBuffer = commands::BeginSingleTimeCommands(vk);

  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = vk.m_ImGuiRenderPass;
  renderPassInfo.framebuffer = vk.m_SwapChainFramebuffers[vk.m_CurrentFrameIndex];
  renderPassInfo.renderArea.offset = { 0,0 };
  renderPassInfo.renderArea.extent = vk.m_SwapChainImageExtent;

  std::array<VkClearValue, 2> clearValues{};
  clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
  clearValues[1].depthStencil = { 1.0f, 0 };

  renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
  renderPassInfo.pClearValues = clearValues.data();

  vkCmdBeginRenderPass(imguiCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), imguiCommandBuffer);
  vkCmdEndRenderPass(imguiCommandBuffer);
  commands::EndSingleTimeCommands(vk, imguiCommandBuffer);
}