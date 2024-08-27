#pragma once

#include <flecs.h>

#include "glm/glm.hpp"

// clang-format off
#include "glad/glad.h"
#include "GLFW/glfw3.h"
// clang-format on

#include <iostream>
#include <map>
#include <vector>


namespace simple
{

// struct device {
// 	static struct device_info {
// 		flecs::entity entity;
// 		GLFWwindow* id;
// 	};

// 	static device_info create() {
// 		GLFWwindow window;
// 	};
// };

struct InputState {
	std::map<int, bool> keyboard;
	std::map<int, bool> mouse;
	glm::dvec2 mouse_scroll;
	glm::dvec2 mouse_pos;
};

struct WindowInstance {
	GLFWwindow* id;
	int width;
	int height;
};

struct GraphicsModule {
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

	GraphicsModule(flecs::world& ecs)
	{
		ecs.module<GraphicsModule>();

		// ecs.component<InputState>();
		// ecs.component<WindowInstance>();

		ecs.observer<WindowInstance>()
			.event(flecs::OnSet)
			.each([&](flecs::entity e, WindowInstance& winstance) {
				glfwSetErrorCallback([](int error, const char* description) {
					std::cerr << "GLFW Error (" << error << "): " << description << std::endl;
				});

				if(!glfwInit())
				{
					std::cerr << "Failed to initialize GLFW" << std::endl;
					return;
				}

				// singleton
				ecs.set<InputState>({});

				glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
				glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
				glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
				glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

				auto w = glfwCreateWindow(640, 480, "1", 0, 0); //TODO: check errors

				if(!w)
				{
					std::cerr << "Failed to create GLFW window" << std::endl;
					glfwTerminate();
					return;
				}

				glfwSetWindowCloseCallback(w, [](GLFWwindow* window) {
					std::cout << "close\n";
					auto ptr = static_cast<flecs::entity*>(glfwGetWindowUserPointer(window));

					// glfwDestroyWindow(window);
					ptr->remove<WindowInstance>();
				});

				glfwSetKeyCallback(
					w, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
						auto ecs = flecs::world(
							reinterpret_cast<flecs::world_t*>(glfwGetWindowUserPointer(window)));
						auto input_state = ecs.get_mut<InputState>();
						input_state->keyboard[key] = (action != GLFW_RELEASE);
					});

				glfwSetMouseButtonCallback(
					w, [](GLFWwindow* window, int button, int action, int mods) {
						auto ecs = flecs::world(
							reinterpret_cast<flecs::world_t*>(glfwGetWindowUserPointer(window)));
						auto input_state = ecs.get_mut<InputState>();
						input_state->mouse[button] = (action != GLFW_RELEASE);
					});

				glfwSetScrollCallback(w, [](GLFWwindow* window, double xoffset, double yoffset) {
					auto ecs = flecs::world(
						reinterpret_cast<flecs::world_t*>(glfwGetWindowUserPointer(window)));
					auto input_state = ecs.get_mut<InputState>();
					input_state->mouse_scroll = {xoffset, yoffset};
				});

				glfwSetCursorPosCallback(w, [](GLFWwindow* window, double xpos, double ypos) {
					auto ecs = flecs::world(
						reinterpret_cast<flecs::world_t*>(glfwGetWindowUserPointer(window)));
					auto input_state = ecs.get_mut<InputState>();
					input_state->mouse_pos = {xpos, ypos};
				});

				// glfwSetWindowUserPointer(w, ecs.get_world());

				glfwSetWindowUserPointer(w, ecs.get_world());
				glfwMakeContextCurrent(w);
				// glewExperimental = true;
				// glewInit(); //TODO: check errors
				if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
				{
					std::cerr << "Failed to initialize GLAD" << std::endl;
					glfwTerminate();
					return;
				}

				// Set the viewport
				glViewport(0, 0, 640, 480);

				// Enable VSync
				glfwSwapInterval(1);

				winstance.id = w;

				// ecs.set<WindowInstance>({.id = w});
			});

		ecs.observer<WindowInstance>()
			.event(flecs::OnRemove)
			.each([&](flecs::entity e, WindowInstance& winstance) {
				auto ptr = glfwGetWindowUserPointer(winstance.id);
				glfwDestroyWindow(winstance.id);
				delete ptr;
			});

		ecs.system<InputState>("Scroll nullifier")
			.kind(flecs::PreUpdate)
			.each([](flecs::entity, InputState& w) { w.mouse_scroll = glm::dvec2(0.0, 0.0); });

		ecs.system<const WindowInstance>("Update windows")
			.kind(flecs::OnUpdate)
			.each([](flecs::entity, const WindowInstance& w) {
				int width, height;
				glfwGetFramebufferSize(w.id, &width, &height);

				glViewport(0, 0, width, height);
				glClear(GL_COLOR_BUFFER_BIT);

				glfwSwapBuffers(w.id);
				glfwPollEvents();
			});
	}
};
} // namespace simple