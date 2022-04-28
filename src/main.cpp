#include "SDL.h"
#include "SDL_syswm.h"

#include "bgfx/bgfx.h"
#include "bgfx/platform.h"

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include <iostream>
#include <fstream>
#include <iostream>
#include <vector>
#include <random>

namespace fileops
{

	inline static std::streamoff stream_size(std::istream& file)
	{
		std::istream::pos_type current_pos = file.tellg();
		if (current_pos == std::istream::pos_type(-1)) {
			return -1;
		}
		file.seekg(0, std::istream::end);
		std::istream::pos_type end_pos = file.tellg();
		file.seekg(current_pos);
		return end_pos - current_pos;
	}

	inline bool stream_read_string(std::istream& file, std::string& fileContents)
	{
		std::streamoff len = stream_size(file);
		if (len == -1) {
			return false;
		}

		fileContents.resize(static_cast<std::string::size_type>(len));

		file.read(&fileContents[0], fileContents.length());
		return true;
	}

	inline bool read_file(const std::string& filename, std::string& fileContents)
	{
		std::ifstream file(filename, std::ios::binary);

		if (!file.is_open()) {
			return false;
		}

		const bool success = stream_read_string(file, fileContents);

		file.close();

		return success;
	}
} // namespace fileops


struct MeshHandle
{
	unsigned int id = -1;
};

struct Object
{
	glm::vec3 position = { 0.0f, 0.0f, 0.0f };
	glm::quat rotation = { 1, 0, 0, 0 };
	glm::vec3 scale = { 1.0f, 1.0f, 1.0f };

	glm::mat4x4 getLocalToWorld() const
	{
		glm::vec3 upVector = glm::vec3(0, 1, 0);
		auto translation = glm::translate(glm::mat4(1.0f), position);
		return translation * glm::mat4x4(rotation) * glm::scale(glm::mat4x4(1), scale);
	}

	MeshHandle meshHandle;
};

struct Mesh
{
	bgfx::VertexBufferHandle vbh;
	bgfx::IndexBufferHandle ibh;
};



constexpr int MAX_MESH_COUNT = 10000;
Mesh meshes[MAX_MESH_COUNT];

MeshHandle createMesh()
{
	static unsigned int currentSlot = 0;
	return MeshHandle{ currentSlot++ };
}

bool isValidMesh(MeshHandle handle)
{
	return handle.id != -1;
}

void setMesh(MeshHandle handle, bgfx::VertexBufferHandle vbh, bgfx::IndexBufferHandle ibh)
{
	meshes[handle.id].ibh = ibh;
	meshes[handle.id].vbh = vbh;
}

void activateMesh(MeshHandle handle)
{
	bgfx::setVertexBuffer(0, meshes[handle.id].vbh);
	bgfx::setIndexBuffer(meshes[handle.id].ibh);
}

static bgfx::ShaderHandle createShader(
	const std::string& shader, const char* name)
{
	const bgfx::Memory* mem = bgfx::copy(shader.data(), shader.size());
	const bgfx::ShaderHandle handle = bgfx::createShader(mem);
	bgfx::setName(handle, name);
	return handle;
}

struct PosColorVertex
{
	float x;
	float y;
	float z;
	uint32_t abgr;
};

static PosColorVertex cube_vertices[] = {
	{-1.0f, 1.0f, 1.0f, 0xff000000},   
	{1.0f, 1.0f, 1.0f, 0xff0000ff},
	{-1.0f, -1.0f, 1.0f, 0xff00ff00},
	{1.0f, -1.0f, 1.0f, 0xff00ffff},
	{-1.0f, 1.0f, -1.0f, 0xffff0000},
	{1.0f, 1.0f, -1.0f, 0xffff00ff},
	{-1.0f, -1.0f, -1.0f, 0xffffff00},
	{1.0f, -1.0f, -1.0f, 0xffffffff},
};

static const uint16_t cube_tri_list[] = {
	0, 1, 2, 1, 3, 2, 4, 6, 5, 5, 6, 7, 0, 2, 4, 4, 2, 6,
	1, 5, 3, 5, 7, 3, 0, 4, 1, 4, 5, 1, 2, 3, 6, 6, 3, 7,
};

void createCube(MeshHandle handle)
{
	bgfx::VertexLayout ms_layout;
	ms_layout
		.begin()
		.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
		.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
		.end();

	auto vbh = bgfx::createVertexBuffer(
		bgfx::makeRef(cube_vertices, sizeof(cube_vertices))
		, ms_layout
	);

	auto ibh = bgfx::createIndexBuffer(
		bgfx::makeRef(cube_tri_list, sizeof(cube_tri_list))
	);
	setMesh(handle, vbh, ibh);
}

struct View
{
	unsigned int id = 0;
	glm::vec3 position = { -10.0f, -5.0f, -30.0f };
	glm::quat rotation = { 1, 0, 0, 0 };
	float fov = 60.0f;
	unsigned int width = 800;
	unsigned int height = 600;

	glm::mat4x4 getViewMat() const
	{
		glm::vec3 upVector = glm::vec3(0, 1, 0);
		auto translation = glm::translate(glm::mat4(1.0f), position);
		return translation * glm::mat4x4(rotation);
	}

	glm::mat4x4 getProjMat() const
	{
		return glm::perspective(glm::radians(60.0f), float(width) / float(height), 0.1f, 100.0f);
	}
};

class RenderPipeline
{
public:
	RenderPipeline()
	{
		std::string vshader;
		if (!fileops::read_file("shaders/v_simple.sc.bin", vshader)) {
			return;
		}

		std::string fshader;
		if (!fileops::read_file("shaders/f_simple.sc.bin", fshader)) {
			return;
		}

		bgfx::ShaderHandle vsh = createShader(vshader, "vshader");
		bgfx::ShaderHandle fsh = createShader(fshader, "fshader");
		program = bgfx::createProgram(vsh, fsh, true);
	}

	void render(View& view, const std::vector<Object>& objects)
	{
		bgfx::setViewTransform(view.id, &view.getViewMat()[0], &view.getProjMat()[0]);

		bgfx::setViewRect(view.id, 0, 0, uint16_t(view.width), uint16_t(view.height));
		bgfx::touch(view.id);

		for (auto& object : objects)
		{
			if (!isValidMesh(object.meshHandle))
				continue;

			activateMesh(object.meshHandle);

			bgfx::setTransform(&object.getLocalToWorld()[0]);

			bgfx::setState(0
				| BGFX_STATE_WRITE_RGB
				| BGFX_STATE_WRITE_Z
				| BGFX_STATE_DEPTH_TEST_LESS
			);
			bgfx::submit(view.id, program);
		}
	}
private:
	unsigned int state = BGFX_STATE_MSAA;
	bgfx::ProgramHandle program = {};
};

int main(int argc, char** argv)
{
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		printf("SDL could not initialize. SDL_Error: %s\n", SDL_GetError());
		return 1;
	}

	const int width = 800;
	const int height = 600;
	SDL_Window* window = SDL_CreateWindow(
		argv[0], SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width,
		height, SDL_WINDOW_SHOWN);

	if (window == nullptr) {
		printf("Window could not be created. SDL_Error: %s\n", SDL_GetError());
		return 1;
	}

	SDL_SysWMinfo wmi;
	SDL_VERSION(&wmi.version);
	if (!SDL_GetWindowWMInfo(window, &wmi)) {
		printf(
			"SDL_SysWMinfo could not be retrieved. SDL_Error: %s\n",
			SDL_GetError());
		return 1;
	}

	bgfx::PlatformData pd{};
	pd.nwh = wmi.info.win.window;
	bgfx::Init bgfx_init;
	bgfx_init.type = bgfx::RendererType::Count; // auto choose renderer
	bgfx_init.resolution.width = width;
	bgfx_init.resolution.height = height;
	bgfx_init.resolution.reset = BGFX_RESET_VSYNC;
	bgfx_init.platformData = pd;
	bgfx_init.deviceId = 1;
	bgfx::init(bgfx_init);

	bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x443355FF, 1.0f, 0);
	bgfx::setViewRect(0, 0, 0, width, height);

	View view0;
	MeshHandle cubeMesh = createMesh();
	createCube(cubeMesh);

	constexpr int meshCount = 25;

	std::vector<Object> objects;
	objects.resize(meshCount);
	for (auto& object : objects)
	{
		object.meshHandle = cubeMesh;
	}

	std::random_device rd;  //Will be used to obtain a seed for the random number engine
	std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
	std::uniform_int_distribution<> distrib(0, 360);

	const int rowCount = 5;
	for (int x = 0; x < rowCount; ++x)
	{
		for (int y = 0; y < rowCount; ++y)
		{
			float angle = distrib(gen);
			int index = x + y * rowCount;
			objects[index].rotation = glm::rotate(objects[index].rotation, glm::radians(angle), glm::vec3(0, 1, 0));
			objects[index].rotation = glm::rotate(objects[index].rotation, glm::radians(angle), glm::vec3(1, 0, 0));
			objects[index].rotation = glm::rotate(objects[index].rotation, glm::radians(angle), glm::vec3(0, 0, 1));
			objects[index].position = { x * 4, y * 4, -4 };
		}
	}


	float direction = 0;

	RenderPipeline renderPipeline;

	bool running = true;
	while (running)
	{
		for (SDL_Event currentEvent; SDL_PollEvent(&currentEvent) != 0;) {
			switch (currentEvent.type) {
			case SDL_QUIT:
				running = false;
				break;
			case SDL_KEYDOWN:
				switch (currentEvent.key.keysym.sym) {
				case SDLK_LEFT:
					view0.position.x -= 1;
					break;
				case SDLK_RIGHT:
					view0.position.x += 1;
					break;
				case SDLK_UP:
					view0.position.y -= 1;
					break;
				case SDLK_DOWN:
					view0.position.y += 1;
					break;
				default:
					break;
				}
				break;

			case SDL_KEYUP:
				break;

			default:
				break;
			}
		}

		auto updateObject = [&](Object& object) {
			object.rotation = glm::rotate(object.rotation, glm::radians(1.0f), glm::vec3(0, 1, 0));
			object.rotation = glm::rotate(object.rotation, glm::radians(1.0f), glm::vec3(1, 0, 0));
			object.rotation = glm::rotate(object.rotation, glm::radians(1.0f), glm::vec3(0, 0, 1));

			object.scale += direction;
			if (object.scale.x > 3)
			{
				object.scale = glm::vec3(3.0f);
				direction *= -1;
			}
			if (object.scale.x < 1)
			{
				object.scale = glm::vec3(1.0f);
				direction *= -1;
			}
		};

		for (auto& object : objects)
			updateObject(object);

		renderPipeline.render(view0, objects);

		bgfx::frame();
	}

	bgfx::shutdown();

	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}