#include "flecs.h"

#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

// #include "vec3.hpp"
#include <iostream>
#include <vector>

// clang-format off
#include "glad/glad.h"
#include "GLFW/glfw3.h"
// clang-format on


// Component types
struct Position {
	float x;
	float y;
	float z;
};

struct Velocity {
	float x;
	float y;
	float z;
};

// Tag types
struct Eats { };
struct Apples { };
struct RendererScope { };

namespace prefabs
{
struct programs {
	struct SimpleProgram { };
	struct ParticleProgram { };
};

struct materials {
	struct Metal { };
	struct Glass { };
};
} // namespace prefabs

namespace renderer
{
// GLFW error callback
void error_callback(int error, const char* description)
{
	std::cerr << "GLFW Error (" << error << "): " << description << std::endl;
}

// Window close callback
void window_close_callback(GLFWwindow* window)
{
	// Get the ECS world from the window user pointer
	flecs::world* ecs = static_cast<flecs::world*>(glfwGetWindowUserPointer(window));

	// Signal the ECS to quit
	ecs->quit();
}

bool check_compile_errors(unsigned int shader, std::string type)
{
	GLint success;
	GLchar infoLog[1024];
	if(type != "PROGRAM")
	{
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if(!success)
		{
			glGetShaderInfoLog(shader, 1024, NULL, infoLog);
			std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n"
					  << infoLog << "\n -- --------------------------------------------------- -- "
					  << std::endl;
			return true;
		}
	}
	else
	{
		glGetProgramiv(shader, GL_LINK_STATUS, &success);
		if(!success)
		{
			glGetProgramInfoLog(shader, 1024, NULL, infoLog);
			std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n"
					  << infoLog << "\n -- --------------------------------------------------- -- "
					  << std::endl;
			return true;
		}
	}
	return false;
}

// 윈도우 컴포넌트
struct WindowComponent {
	GLFWwindow* window;
	int width = 640;
	int height = 480;
	std::string title = "window";
	float scrollOffset = 0.0f;

	static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
	{
		auto windowComponent = static_cast<WindowComponent*>(glfwGetWindowUserPointer(window));
		windowComponent->scrollOffset = yoffset;
	}
};

// 지오메트리 컴포넌트
struct GeometryComponent {
	std::string filePath;
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec2> texCoords;
	std::vector<unsigned int> indices;
	bool hasNormals;
	bool hasTexCoords;
};

// 메쉬 컴포넌트
struct MeshComponent {
	unsigned int VAO;
	unsigned int VBO;
	unsigned int EBO;
	unsigned int vertexCount;
	unsigned int indexCount;
	glm::mat4 model;

	MeshComponent()
	{
		this->model = glm::mat4(1.0f);
	}
};

// 셰이더 컴포넌트
struct ShaderComponent {
	unsigned int program;
	bool compiled;
	std::string vertexShaderSource;
	std::string fragmentShaderSource;
};

struct CameraComponent {
	glm::vec3 position;
	glm::vec3 lookat;
	glm::vec3 up;
	float fov;
	float near_;
	float far_;
	bool ortho;

	CameraComponent()
	{
		this->set_position(0, 0, -1);
		this->set_lookat(0, 0, 0);
		this->set_up(0, 1, 0);
		this->set_fov(30);
		this->near_ = 0.1f;
		this->far_ = 100;
		this->ortho = false;
	}

	void set_position(float x, float y, float z)
	{
		this->position[0] = x;
		this->position[1] = y;
		this->position[2] = z;
	}

	void set_position(glm::vec3 pos)
	{
		this->position = pos;
	}

	void set_lookat(float x, float y, float z)
	{
		this->lookat[0] = x;
		this->lookat[1] = y;
		this->lookat[2] = z;
	}

	void set_lookat(glm::vec3 lookat)
	{
		this->lookat = lookat;
	}

	void set_up(float x, float y, float z)
	{
		this->up[0] = x;
		this->up[1] = y;
		this->up[2] = z;
	}

	void set_up(glm::vec3 up)
	{
		this->up = up;
	}

	void set_fov(float value)
	{
		this->fov = value;
	}
};

struct FPSCameraControllerComponent {
	float moveSpeed = 5.0f;
	float mouseSensitivity = 0.1f;
	glm::vec3 velocity = glm::vec3(0.0f);
	float yaw = -90.0f;
	float pitch = 0.0f;
};

struct OrbitCameraControllerComponent {
	float radius = 5.0f;
	float rotationSpeed = 1.0f;
	float zoomSpeed = 1.0f;
	glm::vec3 target = glm::vec3(0.0f);
	float yaw = -90.0f;
	float pitch = 0.0f;
};

// 셰이더 로드 시스템
void load_shader_system(flecs::iter& it, ShaderComponent* shader)
{
	if(!shader->compiled)
	{
		bool errorOccurred = false;

		// Vertex shader compilation
		unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
		const char* vertexShaderSourceStr = shader->vertexShaderSource.c_str();
		glShaderSource(vertexShader, 1, &vertexShaderSourceStr, nullptr);
		glCompileShader(vertexShader);
		errorOccurred = check_compile_errors(vertexShader, "VERTEX");

		// Fragment shader compilation
		unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		const char* fragmentShaderSourceStr = shader->fragmentShaderSource.c_str();
		glShaderSource(fragmentShader, 1, &fragmentShaderSourceStr, nullptr);
		glCompileShader(fragmentShader);
		errorOccurred = errorOccurred || check_compile_errors(fragmentShader, "FRAGMENT");

		// Shader program creation and linking
		unsigned int shaderProgram = glCreateProgram();
		glAttachShader(shaderProgram, vertexShader);
		glAttachShader(shaderProgram, fragmentShader);
		glLinkProgram(shaderProgram);
		errorOccurred = errorOccurred || check_compile_errors(shaderProgram, "PROGRAM");

		// Delete shaders
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);

		// Set shader component
		shader->program = shaderProgram;
		shader->compiled = true;

		// Print success message only if no errors occurred
		if(!errorOccurred)
		{
			std::cout << "Shader compilation successful!" << std::endl;
		}
	}
}

// 지오메트리 로드 시스템
void load_geometry_system(flecs::iter& it, GeometryComponent* geometry)
{
	if(!geometry->filePath.empty())
	{
		tinyobj::ObjReader reader;
		tinyobj::ObjReaderConfig config;
		// config.triangulate = false;
		// config.vertex_color = false;

		if(!reader.ParseFromFile(geometry->filePath, config))
		{
			if(!reader.Error().empty())
			{
				std::cerr << "TinyObjReader: " << reader.Error();
			}
			return;
		}

		if(!reader.Warning().empty())
		{
			std::cout << "TinyObjReader: " << reader.Warning();
		}

		auto& attrib = reader.GetAttrib();
		auto& shapes = reader.GetShapes();

		// 버텍스 레이아웃 구성
		std::vector<tinyobj::real_t> vertexData;
		geometry->hasNormals = !attrib.normals.empty();
		geometry->hasTexCoords = !attrib.texcoords.empty();

		// 버텍스 데이터 추출
		for(const auto& shape : shapes)
		{
			for(const auto& index : shape.mesh.indices)
			{
				glm::vec3 vertex{attrib.vertices[3 * index.vertex_index + 0],
								 attrib.vertices[3 * index.vertex_index + 1],
								 attrib.vertices[3 * index.vertex_index + 2]};
				geometry->vertices.push_back(vertex);

				if(index.normal_index >= 0)
				{
					glm::vec3 normal{attrib.normals[3 * index.normal_index + 0],
									 attrib.normals[3 * index.normal_index + 1],
									 attrib.normals[3 * index.normal_index + 2]};
					geometry->normals.push_back(normal);
				}

				if(index.texcoord_index >= 0)
				{
					glm::vec2 texCoord{attrib.texcoords[2 * index.texcoord_index + 0],
									   attrib.texcoords[2 * index.texcoord_index + 1]};
					geometry->texCoords.push_back(texCoord);
				}
			}
		}

		// 인덱스 데이터 추출
		for(size_t i = 0; i < geometry->vertices.size(); ++i)
		{
			geometry->indices.push_back(i);
		}

		// 파일 경로 초기화
		geometry->filePath.clear();
	}
}

// 메쉬 생성 시스템
void create_mesh_system(flecs::iter& it, renderer::MeshComponent* mesh)
{
	// VAO 생성
	glGenVertexArrays(1, &mesh->VAO);
	glBindVertexArray(mesh->VAO);

	// VBO 생성
	glGenBuffers(1, &mesh->VBO);
	glBindBuffer(GL_ARRAY_BUFFER, mesh->VBO);

	// EBO 생성
	glGenBuffers(1, &mesh->EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->EBO);

	// VAO 언바인딩
	glBindVertexArray(0);

	// Unbind the VBO and EBO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

// 메쉬 로드 시스템
void load_mesh_system(flecs::iter& it, GeometryComponent* geometry, MeshComponent* mesh)
{
	// 버텍스 버퍼 데이터 준비
	std::vector<float> vertexData;
	for(size_t i = 0; i < geometry->vertices.size(); ++i)
	{
		vertexData.push_back(geometry->vertices[i].x);
		vertexData.push_back(geometry->vertices[i].y);
		vertexData.push_back(geometry->vertices[i].z);

		if(geometry->hasNormals && i < geometry->normals.size())
		{
			vertexData.push_back(geometry->normals[i].x);
			vertexData.push_back(geometry->normals[i].y);
			vertexData.push_back(geometry->normals[i].z);
		}

		if(geometry->hasTexCoords && i < geometry->texCoords.size())
		{
			vertexData.push_back(geometry->texCoords[i].x);
			vertexData.push_back(geometry->texCoords[i].y);
		}
	}

	// VAO 바인딩
	glBindVertexArray(mesh->VAO);

	// 버텍스 버퍼 업데이트
	glBindBuffer(GL_ARRAY_BUFFER, mesh->VBO);
	glBufferData(
		GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);

	// 인덱스 버퍼 업데이트
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
				 geometry->indices.size() * sizeof(unsigned int),
				 geometry->indices.data(),
				 GL_STATIC_DRAW);

	// 버텍스 속성 설정
	int stride = 3 * sizeof(float);
	if(geometry->hasNormals)
		stride += 3 * sizeof(float);
	if(geometry->hasTexCoords)
		stride += 2 * sizeof(float);

	int offset = 0;

	// 위치 속성
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offset);
	glEnableVertexAttribArray(0);
	offset += 3 * sizeof(float);

	// 법선 속성
	if(geometry->hasNormals)
	{
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)offset);
		glEnableVertexAttribArray(1);
		offset += 3 * sizeof(float);
	}

	// 텍스처 좌표 속성
	if(geometry->hasTexCoords)
	{
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)offset);
		glEnableVertexAttribArray(2);
	}

	// VAO 언바인딩
	glBindVertexArray(0);

	// Unbind the VBO and EBO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// 버텍스 개수 및 인덱스 개수 업데이트
	mesh->vertexCount = geometry->vertices.size();
	mesh->indexCount = geometry->indices.size();
}

void fps_camera_controller_system(flecs::iter& it,
								  CameraComponent* camera,
								  FPSCameraControllerComponent* controller)
{
	auto window = it.world().get<WindowComponent>();

	// 마우스 입력 처리
	double mouseX, mouseY;
	glfwGetCursorPos(window->window, &mouseX, &mouseY);

	float xoffset = mouseX - window->width / 2;
	float yoffset = window->height / 2 - mouseY;

	controller->yaw += xoffset * controller->mouseSensitivity;
	controller->pitch += yoffset * controller->mouseSensitivity;

	controller->pitch = glm::clamp(controller->pitch, -89.0f, 89.0f);

	glm::vec3 direction;
	direction.x = cos(glm::radians(controller->yaw)) * cos(glm::radians(controller->pitch));
	direction.y = sin(glm::radians(controller->pitch));
	direction.z = sin(glm::radians(controller->yaw)) * cos(glm::radians(controller->pitch));
	camera->lookat = glm::normalize(direction);

	glm::vec3 right = glm::normalize(glm::cross(camera->lookat, camera->up));

	// 키보드 입력 처리
	controller->velocity = glm::vec3(0.0f);
	if(glfwGetKey(window->window, GLFW_KEY_W) == GLFW_PRESS)
		controller->velocity += camera->lookat;
	if(glfwGetKey(window->window, GLFW_KEY_S) == GLFW_PRESS)
		controller->velocity -= camera->lookat;
	if(glfwGetKey(window->window, GLFW_KEY_A) == GLFW_PRESS)
		controller->velocity -= right;
	if(glfwGetKey(window->window, GLFW_KEY_D) == GLFW_PRESS)
		controller->velocity += right;
	if(glfwGetKey(window->window, GLFW_KEY_Q) == GLFW_PRESS)
		controller->velocity -= camera->up;
	if(glfwGetKey(window->window, GLFW_KEY_E) == GLFW_PRESS)
		controller->velocity += camera->up;

	// 속도 및 관성 적용
	camera->position += controller->velocity * controller->moveSpeed * it.delta_time();
	controller->velocity *= 0.9f;

	glfwSetCursorPos(window->window, window->width / 2, window->height / 2);
}

void orbit_camera_controller_system(flecs::iter& it,
									CameraComponent* camera,
									OrbitCameraControllerComponent* controller)
{
	auto window = it.world().get_mut<WindowComponent>();

	if(glfwGetMouseButton(window->window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
	{
		double mouseX, mouseY;
		glfwGetCursorPos(window->window, &mouseX, &mouseY);

		float xoffset = mouseX - window->width / 2;
		float yoffset = window->height / 2 - mouseY;

		controller->yaw += xoffset * controller->rotationSpeed;
		controller->pitch += yoffset * controller->rotationSpeed;

		controller->pitch = glm::clamp(controller->pitch, -89.0f, 89.0f);
	}

	// 카메라 위치 계산
	glm::vec3 direction;
	direction.x = cos(glm::radians(controller->yaw)) * cos(glm::radians(controller->pitch));
	direction.y = sin(glm::radians(controller->pitch));
	direction.z = sin(glm::radians(controller->yaw)) * cos(glm::radians(controller->pitch));
	camera->position = controller->target + direction * controller->radius;

	camera->lookat = controller->target;

	// 마우스 스크롤 휠 처리
	controller->radius -= window->scrollOffset * controller->zoomSpeed;
	controller->radius = glm::clamp(controller->radius, 1.0f, 100.0f);
	window->scrollOffset = 0.0f;

	glfwSetCursorPos(window->window, window->width / 2, window->height / 2);
}

// 렌더링 시스템
void render_system(flecs::iter& it, const ShaderComponent* shader, const MeshComponent* mesh)
{
	auto cameraEntity = it.world().lookup("camera");
	if(!cameraEntity.is_alive())
	{
		// 카메라 엔티티가 존재하지 않는 경우 처리
		return;
	}
	auto camera = cameraEntity.get<CameraComponent>();

	// 현재 윈도우 가져오기
	GLFWwindow* window = it.world().get<WindowComponent>()->window;

	// 윈도우가 닫히지 않았는지 확인
	if(!glfwWindowShouldClose(window))
	{
		// 윈도우 크기 가져오기
		int width, height;
		glfwGetWindowSize(window, &width, &height);

		glm::mat4 model = mesh->model;
		glm::mat4 view = glm::lookAt(camera->position, camera->lookat, camera->up);
		glm::mat4 projection;

		if(camera->ortho)
		{
			float aspect = (float)width / (float)height;
			projection = glm::ortho(-aspect, aspect, -1.0f, 1.0f, camera->near_, camera->far_);
		}
		else
		{
			projection = glm::perspective(glm::radians(camera->fov),
										  (float)width / (float)height,
										  camera->near_,
										  camera->far_);
		}

		// 배경색 설정
		glClearColor(0.8f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// 셰이더 프로그램 사용
		glUseProgram(shader->program);

		GLint model_location = glGetUniformLocation(shader->program, "model");
		GLint view_location = glGetUniformLocation(shader->program, "view");
		GLint projection_location = glGetUniformLocation(shader->program, "projection");

		glUniformMatrix4fv(model_location, 1, GL_FALSE, glm::value_ptr(model));
		glUniformMatrix4fv(view_location, 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(projection_location, 1, GL_FALSE, glm::value_ptr(projection));

		// VAO 바인딩
		glBindVertexArray(mesh->VAO);

		// 삼각형 그리기
		glDrawElements(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, 0);

		// VAO 언바인딩
		glBindVertexArray(0);

		// 더블 버퍼링
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}

// 윈도우 초기화 시스템
void window_init_system(flecs::iter& it)
{
	// Set the GLFW error callback
	glfwSetErrorCallback(error_callback);

	// Initialize GLFW
	if(!glfwInit())
	{
		std::cerr << "Failed to initialize GLFW" << std::endl;
		return;
	}

	// Set OpenGL version to 3.0
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Get the WindowComponent
	auto windowComponent = it.world().get_mut<WindowComponent>();

	// Create the window
	GLFWwindow* window = glfwCreateWindow(windowComponent->width,
										  windowComponent->height,
										  windowComponent->title.c_str(),
										  nullptr,
										  nullptr);
	if(!window)
	{
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return;
	}

	// Set the window close callback
	glfwSetWindowCloseCallback(window, window_close_callback);

	glfwSetScrollCallback(window, WindowComponent::scrollCallback);

	// Set the ECS world as the window user pointer
	glfwSetWindowUserPointer(window, it.world());

	// Make the window's context current
	glfwMakeContextCurrent(window);

	// Initialize GLAD
	if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cerr << "Failed to initialize GLAD" << std::endl;
		glfwTerminate();
		return;
	}

	// Set the viewport
	glViewport(0, 0, windowComponent->width, windowComponent->height);

	// Enable VSync
	glfwSwapInterval(1);

	// Set the window in the WindowComponent
	windowComponent->window = window;
}

// 메인 루프 시스템
void main_loop_system(flecs::iter& it)
{
	// ... (메인 루프 코드는 동일)
}

// 윈도우 종료 시스템
void window_shutdown_system(flecs::iter& it)
{
	// ... (윈도우 종료 코드는 동일)
}

void init_shaders(flecs::world& ecs)
{
	{
		// Vertex Shader
		const char* vertexShaderSource = R"(
#version 300 es
precision mediump float;

layout (location = 0) in vec3 aPos;

void main()
{
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}
)";

		// Fragment Shader
		const char* fragmentShaderSource = R"(
#version 300 es
precision mediump float;

out vec4 FragColor;

void main()
{
    FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
}
)";

		// Create a ShaderComponent
		renderer::ShaderComponent shaderComponent;
		shaderComponent.vertexShaderSource = vertexShaderSource;
		shaderComponent.fragmentShaderSource = fragmentShaderSource;

		// Add the ShaderComponent to the SimpleProgram prefab
		ecs.prefab<prefabs::programs::SimpleProgram>().set<renderer::ShaderComponent>(
			shaderComponent);
	}
}

// 초기화 시스템
void init_systems(flecs::world& ecs)
{
	ecs.scope<RendererScope>([&]() { // Keep root scope clean
		// 윈도우 초기화 시스템 등록
		ecs.system<>("WindowInitSystem").kind(flecs::OnStart).iter(window_init_system);

		// 셰이더 로드 시스템 등록
		ecs.system<ShaderComponent>("LoadShaderSystem")
			.kind(flecs::OnStart)
			.iter(load_shader_system);

		// 지오메트리 로드 시스템 등록
		ecs.system<GeometryComponent>("LoadGeometrySystem")
			.kind(flecs::OnStart)
			.iter(load_geometry_system);

		// 메쉬 생성 시스템 등록
		ecs.system<MeshComponent>("CreateMeshSystem").kind(flecs::OnStart).iter(create_mesh_system);

		// 메쉬 로드 시스템 등록
		ecs.system<GeometryComponent, MeshComponent>("LoadMeshSystem")
			.kind(flecs::OnStart)
			.iter(load_mesh_system);

		// 렌더링 시스템 등록
		ecs.system<ShaderComponent, MeshComponent>("RenderSystem")
			// .term_at(3)
			// .singleton()
			.kind(flecs::OnUpdate)
			.iter(render_system);

		// 메인 루프 시스템 등록
		ecs.system<WindowComponent>("MainLoopSystem").kind(flecs::OnUpdate).iter(main_loop_system);

		// ecs.system<CameraComponent, FPSCameraControllerComponent>("FPSCameraControllerComponent")
		// 	.kind(flecs::OnUpdate)
		// 	.iter(fps_camera_controller_system);

		// ecs.system<CameraComponent, OrbitCameraControllerComponent>(
		// 	   "OrbitCameraControllerComponent")
		// 	.kind(flecs::OnUpdate)
		// 	.iter(orbit_camera_controller_system);

		// 윈도우 종료 시스템 등록
		// ecs.system<WindowComponent>()
		//     .kind(flecs::OnUnload)
		//     .iter(window_shutdown_system);
	});
}
} // namespace renderer

int main(int argc, char* argv[])
{
	// Create the world
	flecs::world world(argc, argv);
	flecs::log::set_level(1);

	world.import <flecs::monitor>();

	renderer::init_systems(world);

	// // Register system
	world.system<Position, Velocity>().each([](Position& p, Velocity& v) {
		p.x += v.x;
		p.y += v.y;
	});

	world.system<Position, Velocity>().iter([](flecs::iter& it, Position* p, Velocity* v) {
		for(auto i : it)
		{
			std::cout << "Moved " << it.entity(i).name() << " to {" << p[i].x << ", " << p[i].y
					  << "}" << std::endl;
		}
	});

	// // Create an entity with name Bob, add Position and food preference
	flecs::entity Bob =
		world.entity("Bob").set(Position{0, 0}).set(Velocity{1, 2}).add<Eats, Apples>();

	// // Show us what you got
	// std::cout << Bob.name() << "'s got [" << Bob.type().str() << "]\n";

	// // See if Bob has moved (he has)
	// const Position* p = Bob.get<Position>();
	// std::cout << Bob.name() << "'s position is {" << p->x << ", " << p->y << "}\n";

	// Vertex Shader
	const char* vertexShaderSource = R"(
#version 300 es
precision mediump float;

layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

	// Fragment Shader
	const char* fragmentShaderSource = R"(
#version 300 es
precision mediump float;

out vec4 FragColor;

void main()
{
    FragColor = vec4(1.0f, 0.5f, 0.6f, 1.0f);
}
)";
	renderer::ShaderComponent shaderComponent;
	shaderComponent.vertexShaderSource = vertexShaderSource;
	shaderComponent.fragmentShaderSource = fragmentShaderSource;
	{

		renderer::GeometryComponent geometryComponent;
		geometryComponent.filePath = "C:/Users/user/Downloads/teapot.obj";

		flecs::entity renderable1 = world.entity("renderable1")
										.set<renderer::GeometryComponent>(geometryComponent)
										.add<renderer::MeshComponent>()
										.set<renderer::ShaderComponent>(shaderComponent)
										.set<Position>({0, -1, 0});
	}

	// auto camera = world.get_mut<renderer::CameraComponent>();

	renderer::CameraComponent cameraComponent;
	cameraComponent.set_position({0, 0, -10});
	cameraComponent.set_lookat({0, 0, 0});

	flecs::entity camera = world.entity("camera")
							   .set<renderer::CameraComponent>(cameraComponent)
							   .add<renderer::OrbitCameraControllerComponent>();
	//    .add<renderer::FPSCameraControllerComponent>();

	// world.set<renderer::CameraComponent>({});

	// auto aa = world.get_mut<renderer::CameraComponent>();

	// world.set_target_fps(1);
	// while(!world.should_quit())
	// {
	// 	world.progress();
	// }

	// glfwTerminate();

	world.app().enable_rest().target_fps(60).run();
}