#include "pane.hpp"

std::unique_ptr<nri::GraphicsProgram> fxed::Pane::backgroundShader = nullptr;
std::unique_ptr<fxed::QuadMesh>		  fxed::Pane::backgroundMesh   = nullptr;

struct PushConstants {
	glm::vec3  color0;
	int		   borderSize;
	glm::vec3  color1;
	float	   time;
	glm::ivec2 viewportSize;
};

fxed::Pane::Pane(nri::NRI &nri, nri::CommandQueue &queue) {
	if (!backgroundShader) {
		auto sb = nri.createProgramBuilder();
		backgroundShader =
			sb->addShaderModule(nri::ShaderCreateInfo{"shaders/pane.hlsl", "VSMain", nri::SHADER_TYPE_VERTEX})
				.addShaderModule(nri::ShaderCreateInfo{"shaders/pane.hlsl", "PSMain", nri::SHADER_TYPE_FRAGMENT})
				.setVertexBindings(fxed::QuadMesh::getVertexBindings())
				.setPrimitiveType(nri::PRIMITIVE_TYPE_TRIANGLES)
				.setPushConstantRanges({{0, sizeof(PushConstants)}})
				.buildGraphicsProgram();
	}
	if (!backgroundMesh) { backgroundMesh = std::make_unique<fxed::QuadMesh>(nri, queue, glm::vec2{2.0, 2.0}); }
}

void fxed::Pane::render(nri::CommandBuffer &cmdBuf) {
	backgroundShader->bind(cmdBuf);
	backgroundMesh->bind(cmdBuf);

	PushConstants pushConstants{
		.color0		  = glm::vec3(30 / 255.f, 30 / 255.f, 46 / 255.f),
		.borderSize	  = 2,
		.color1		  = glm::vec3(1.0f, 1.0f, 1.0f),
		.time		  = 0,
		.viewportSize = {800, 600},
	};

	backgroundShader->setPushConstants(cmdBuf, &pushConstants, sizeof(pushConstants), 0);

	backgroundMesh->draw(cmdBuf, *backgroundShader);
}
