#include "Framework/Framework.h"

using namespace LavaCake;
using namespace LavaCake::Geometry;
using namespace LavaCake::Framework;
using namespace LavaCake::Core;


#ifdef __APPLE__
std::string prefix ="../";
#else
std::string prefix ="";
#endif

GraphicPipeline* pipeline;
VertexBuffer* triangle_vertex_buffer;

RenderPass* createRenderPass(Queue * queue, CommandBuffer& commandBuffer) {
	SwapChain* s = SwapChain::getSwapChain();
	VkExtent2D size = s->size();
	//We define the stride format we need for the mesh here 
	//each vertex is a 3D position followed by a RGB color
	



	VertexShaderModule* vertexShader = new VertexShaderModule(prefix + "Data/Shaders/helloworld/shader.vert.spv");
	FragmentShaderModule* fragmentShader = new FragmentShaderModule(prefix + "Data/Shaders/helloworld/shader.frag.spv");

	pipeline = new GraphicPipeline(vec3f({ 0,0,0 }), vec3f({ float(size.width),float(size.height),1.0f }), vec2f({ 0,0 }), vec2f({ float(size.width),float(size.height) }));
	pipeline->setVertexModule(vertexShader);
	pipeline->setFragmentModule(fragmentShader);
	pipeline->setVerticesInfo(triangle_vertex_buffer->getBindingDescriptions(), triangle_vertex_buffer->getAttributeDescriptions() ,triangle_vertex_buffer->primitiveTopology());
	


	SubpassAttachment SA;
	SA.showOnScreen = true;
	SA.nbColor = 1;
	SA.storeColor = true;
	SA.useDepth = true;
	SA.showOnScreenIndex = 0;

	RenderPass* pass = new RenderPass();
	pass->addSubPass({ pipeline }, SA);
	pass->addDependencies(VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_MEMORY_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_DEPENDENCY_BY_REGION_BIT);
	pass->addDependencies(0, VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, VK_DEPENDENCY_BY_REGION_BIT);
	pass->compile();

	return pass;
	
}

bool g_resize = false;

void window_size_callback(GLFWwindow* window, int width, int height)
{
	g_resize = true;
}



int main() {

	Window w("LavaCake HelloWorld", 512, 512);
	ErrorCheck::PrintError(true);


	glfwSetWindowSizeCallback(w.m_window, window_size_callback);
	
	Device* d = Device::getDevice();
	d->initDevices(0, 1, w.m_windowParams);
	SwapChain* s = SwapChain::getSwapChain();
	s->init();

	VkExtent2D size = s->size();
	Queue* queue = d->getGraphicQueue(0);
	PresentationQueue* presentQueue = d->getPresentQueue();
	CommandBuffer  commandBuffer;
	commandBuffer.addSemaphore();


	vertexFormat format = vertexFormat({ POS3,COL3 });

	//we create a indexed triangle mesh with the desired format
	Mesh_t* triangle = new IndexedMesh<TRIANGLE>(format);

	//adding 3 vertices
	triangle->appendVertex({ -0.75f, 0.75f, 0.0f, 1.0f , 0.0f , 0.0f });
	triangle->appendVertex({ 0.75f,	0.75f , 0.0f, 0.0f , 1.0f	, 0.0f });
	triangle->appendVertex({ 0.0f , -0.75f, 0.0f, 0.0f , 0.0f	, 1.0f });


	// we link the 3 vertices to define a triangle
	triangle->appendIndex(0);
	triangle->appendIndex(1);
	triangle->appendIndex(2);


	//creating an allocating a vertex buffer
	triangle_vertex_buffer = new VertexBuffer({ triangle });
	triangle_vertex_buffer->allocate(queue, commandBuffer);


	auto pass = createRenderPass(queue, commandBuffer);
	
	FrameBuffer* frameBuffers = new FrameBuffer(size.width, size.height);
	pass->prepareOutputFrameBuffer(*frameBuffers);

	while (w.running()) {
		w.updateInput();
	
		VkDevice logical = d->getLogicalDevice();
		VkSwapchainKHR& swapchain = s->getHandle();
		

		commandBuffer.wait(2000000000);
		commandBuffer.resetFence();

		if (g_resize) {
			d->waitForAllCommands();
			s->resize();
			size = s->size();
			delete pass;
			pass = createRenderPass(queue, commandBuffer);
			delete frameBuffers;
			frameBuffers = new FrameBuffer(size.width, size.height);
			pass->prepareOutputFrameBuffer(*frameBuffers);

			g_resize = false;
		}

		SwapChainImage& image = s->acquireImage();

		std::vector<WaitSemaphoreInfo> wait_semaphore_infos = {};
		wait_semaphore_infos.push_back({
			image.getSemaphore(),                     // VkSemaphore            Semaphore
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT					// VkPipelineStageFlags   WaitingStage
			});

		pipeline->setVertices({ triangle_vertex_buffer });

		commandBuffer.beginRecord();

		
		pass->setSwapChainImage(*frameBuffers, image);

		pass->draw(commandBuffer, *frameBuffers, vec2u({ 0,0 }), vec2u({ size.width, size.height }), { { 0.1f, 0.2f, 0.3f, 1.0f }, { 1.0f, 0 } });

		commandBuffer.endRecord();

		commandBuffer.submit(queue, wait_semaphore_infos, { commandBuffer.getSemaphore(0) });

		s->presentImage(presentQueue, image, { commandBuffer.getSemaphore(0) });
	}
	d->end();
}
