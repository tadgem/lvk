#include "lvk/Structs.h"
bool lvk::QueueFamilyIndices::IsComplete() {
  bool foundGraphicsQueue = m_QueueFamilies.find(QueueFamilyType::GraphicsAndCompute) != m_QueueFamilies.end();
  bool foundPresentQueue  = m_QueueFamilies.find(QueueFamilyType::Present) != m_QueueFamilies.end();
  return foundGraphicsQueue && foundPresentQueue;
}