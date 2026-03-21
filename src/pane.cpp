#include "pane.hpp"

struct PushConstants {
	glm::vec3  color0;
	int		   borderSize;
	glm::vec3  color1;
	float	   time;
	glm::ivec2 viewportSize;
};

fxed::ResourceID fxed::Pane::backgroundShaderID = fxed::ResourceID::invalid();
fxed::ResourceID fxed::Pane::backgroundMeshID	= fxed::ResourceID::invalid();

fxed::Pane::Pane(nri::NRI &nri, nri::CommandQueue &queue) {
	if (backgroundShaderID.invalid()) {
		auto sb = nri.createProgramBuilder();
		auto backgroundShader =
			sb->addShaderModule(nri::ShaderCreateInfo{"shaders/pane.hlsl", "VSMain", nri::SHADER_TYPE_VERTEX})
				.addShaderModule(nri::ShaderCreateInfo{"shaders/pane.hlsl", "PSMain", nri::SHADER_TYPE_FRAGMENT})
				.setVertexBindings(fxed::QuadMesh::getVertexBindings())
				.setPrimitiveType(nri::PRIMITIVE_TYPE_TRIANGLES)
				.setPushConstantRanges({{0, sizeof(PushConstants)}})
				.buildGraphicsProgram();
		backgroundShaderID = fxed::ResourceManager::getInstance().addShader(std::move(backgroundShader));
	}
	if (backgroundMeshID.invalid()) {
		auto backgroundMesh = std::make_unique<fxed::QuadMesh>(nri, queue, glm::vec2{2.0, 2.0});
		backgroundMeshID	= fxed::ResourceManager::getInstance().addMesh(std::move(backgroundMesh));
	}
}

void fxed::Pane::render(nri::CommandBuffer &cmdBuf) {
	auto &backgroundShader = fxed::ResourceManager::getInstance().getShader(backgroundShaderID);
	auto &backgroundMesh   = fxed::ResourceManager::getInstance().getMesh(backgroundMeshID);

	backgroundShader.bind(cmdBuf);
	backgroundMesh.bind(cmdBuf);

	PushConstants pushConstants{
		.color0		  = glm::vec3(30 / 255.f, 30 / 255.f, 46 / 255.f),
		.borderSize	  = 2,
		.color1		  = glm::vec3(1.0f, 1.0f, 1.0f),
		.time		  = 0,
		.viewportSize = {800, 600},
	};

	backgroundShader.setPushConstants(cmdBuf, &pushConstants, sizeof(pushConstants), 0);

	backgroundMesh.draw(cmdBuf, backgroundShader);
}
