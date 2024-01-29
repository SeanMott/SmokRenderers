#pragma once

//defines materials for the various renderers

#include <SmokGraphics/Pipeline/GraphicsPipeline.hpp>

namespace Smok::Renderers::Material
{
	//defines the material type
	enum class MaterialType
	{
		Unlit = 0,

		Count
	};

	//defines a unlit mesh material
	struct UnlitMeshMaterial
	{
		Graphics::Pipeline::GraphicsShader* grapphicsShader = nullptr;
		Graphics::Pipeline::GraphicsPipeline* graphicsPipeline = nullptr;
		//texture

		std::string graphicsShaderDeclFilePath = "",
			graphicsPiipelineDeclFilePath = "";
	};

	//writes a decl file for a unlit material
	inline void UnlitMaterial_WriteDeclFile(UnlitMeshMaterial* material,
		const std::string& assetName, const std::string declFileOutputDir)
	{
		YAML::Emitter emitter;

		emitter << YAML::BeginMap;

		emitter << YAML::Key << "assetName" << YAML::DoubleQuoted << assetName;

		emitter << YAML::Key << "GShaderDeclPath" << YAML::DoubleQuoted << material->graphicsShaderDeclFilePath;
		emitter << YAML::Key << "GPipelineDeclPath" << YAML::DoubleQuoted << material->graphicsPiipelineDeclFilePath;

		emitter << YAML::EndMap;

		BTD::IO::File file; file.Open(declFileOutputDir, BTD::IO::FileOP::TextWrite_OpenCreateStart);
		file.Write(emitter.c_str());
		file.Close();
	}

	//loads a decl file for a unlit material

	//defines a phong based lit mesh material

	//defines a 2D text material

	//defines a Wind Waker Cell Shaded lit mesh material
}