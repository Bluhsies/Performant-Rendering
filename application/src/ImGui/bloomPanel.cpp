//#include "include/ImGui/bloomPanel.hpp"
//
//void BloomPanel::onImGuiRender()
//{
//	if (ImGui::TreeNode("Bloom controls"))
//	{
//		if (ImGui::DragFloat("Bloom Th", &bloomThreshold, 0.05f, 50.f, 0.f))
//		{
//			auto& thresholdMaterial = m_renderer.getRenderPass(1).scene->m_actors[0].material;
//			thresholdMaterial->setValue("u_threshold", bloomThreshold);
//		}
//
//		if (ImGui::TreeNode("Threshold Texture"))
//		{
//			auto bloomImgId = m_renderer.getRenderPass(1).target->getTarget(0)->getID();
//			ImGui::Image(ImTextureID(bloomImgId), ImVec2(750, 600));
//			ImGui::TreePop();
//		}
//
//		if (ImGui::TreeNode("Downsample Textures Tabbar"))
//		{
//			ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
//			if (ImGui::BeginTabBar("Downsample Textures", tab_bar_flags))
//			{
//				if (ImGui::BeginTabItem("Downsample 1"))
//				{
//					auto bloomImgId = m_renderer.getRenderPass(2).target->getTarget(0)->getID();
//					ImGui::Image(ImTextureID(bloomImgId), ImVec2(750, 600));
//					ImGui::EndTabItem();
//				}
//				if (ImGui::BeginTabItem("Downsample 2"))
//				{
//					auto bloomImgId = m_renderer.getRenderPass(3).target->getTarget(0)->getID();
//					ImGui::Image(ImTextureID(bloomImgId), ImVec2(750, 600));
//					ImGui::EndTabItem();
//				}
//				if (ImGui::BeginTabItem("Downsample 3"))
//				{
//					auto bloomImgId = m_renderer.getRenderPass(4).target->getTarget(0)->getID();
//					ImGui::Image(ImTextureID(bloomImgId), ImVec2(750, 600));
//					ImGui::EndTabItem();
//				}
//				if (ImGui::BeginTabItem("Downsample 4"))
//				{
//					auto bloomImgId = m_renderer.getRenderPass(5).target->getTarget(0)->getID();
//					ImGui::Image(ImTextureID(bloomImgId), ImVec2(750, 600));
//					ImGui::EndTabItem();
//				}
//				if (ImGui::BeginTabItem("Downsample 5"))
//				{
//					auto bloomImgId = m_renderer.getRenderPass(6).target->getTarget(0)->getID();
//					ImGui::Image(ImTextureID(bloomImgId), ImVec2(750, 600));
//					ImGui::EndTabItem();
//				}
//				if (ImGui::BeginTabItem("Downsample 6"))
//				{
//					auto bloomImgId = m_renderer.getRenderPass(7).target->getTarget(0)->getID();
//					ImGui::Image(ImTextureID(bloomImgId), ImVec2(750, 600));
//					ImGui::EndTabItem();
//				}
//				ImGui::EndTabBar();
//			}
//			ImGui::TreePop();
//		}
//
//		if (ImGui::TreeNode("Upsample Textures"))
//		{
//			ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
//			if (ImGui::BeginTabBar("Upsample Textures Tabbar", tab_bar_flags))
//			{
//				if (ImGui::BeginTabItem("Upsample 1"))
//				{
//					auto bloomImgId = m_renderer.getRenderPass(8).target->getTarget(0)->getID();
//					ImGui::Image(ImTextureID(bloomImgId), ImVec2(750, 600));
//					ImGui::EndTabItem();
//				}
//				if (ImGui::BeginTabItem("Upsample 2"))
//				{
//					auto bloomImgId = m_renderer.getRenderPass(9).target->getTarget(0)->getID();
//					ImGui::Image(ImTextureID(bloomImgId), ImVec2(750, 600));
//					ImGui::EndTabItem();
//				}
//				if (ImGui::BeginTabItem("Upsample 3"))
//				{
//					auto bloomImgId = m_renderer.getRenderPass(10).target->getTarget(0)->getID();
//					ImGui::Image(ImTextureID(bloomImgId), ImVec2(750, 600));
//					ImGui::EndTabItem();
//				}
//				if (ImGui::BeginTabItem("Upsample 4"))
//				{
//					auto bloomImgId = m_renderer.getRenderPass(11).target->getTarget(0)->getID();
//					ImGui::Image(ImTextureID(bloomImgId), ImVec2(750, 600));
//					ImGui::EndTabItem();
//				}
//				if (ImGui::BeginTabItem("Upsample 5"))
//				{
//					auto bloomImgId = m_renderer.getRenderPass(12).target->getTarget(0)->getID();
//					ImGui::Image(ImTextureID(bloomImgId), ImVec2(750, 600));
//					ImGui::EndTabItem();
//				}
//
//				ImGui::EndTabBar();
//			}
//			ImGui::TreePop();
//		}
//
//		ImGui::Separator();
//		ImGui::TreePop();
//	}
//}
