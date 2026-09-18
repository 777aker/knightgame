#include "../window/window.hpp"

#include <sys/socket.h>
#include <iostream>
#include <arpa/inet.h>
#include <unistd.h>

/**
 * @brief respond to key pressed
 *
 * @param windowobj
 * @param key
 * @param scancode
 * @param action
 * @param mods
 */
void key(GLFWwindow *windowobj, int key, [[maybe_unused]] int scancode, int action,
	 [[maybe_unused]] int mods)
{
	if (action == GLFW_RELEASE) {
		return;
	}

	switch (key) {
	case GLFW_KEY_ESCAPE:
		glfwSetWindowShouldClose(windowobj, 1);
		break;
	}
}

void get_packet()
{
	int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (socket_fd < 0) {
		std::cerr << "Failed to open socket\n";
		return;
	}

	sockaddr_in server_address{};
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(7742);
	server_address.sin_addr.s_addr = INADDR_ANY;

	if (bind(socket_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
		std::cerr << "Bind failed\n";
		close(socket_fd);
		return;
	}

	char buffer[1024];
	sockaddr_in client_address{};
	socklen_t client_len = sizeof(client_address);

	std::cout << "Waiting for bytes\n";
	ssize_t bytes_received = recvfrom(socket_fd, buffer, sizeof(buffer) - 1, 0,
					  (struct sockaddr *)&client_address, &client_len);
	if (bytes_received < 0) {
		std::cerr << "Error receiving data\n";
		close(socket_fd);
		return;
	}

	buffer[bytes_received] = '\0';
	std::cout << "Packet data: " << buffer << "\n";
	close(socket_fd);
}

/**
 * @brief main display loop
 *
 * @param windowobj
 */
void display_loop(Window *windowobj)
{
	while (!glfwWindowShouldClose(windowobj->glwindow)) {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// want to see fps
		glColor3ub(nephritis.r, nephritis.g, nephritis.b);
		glRasterPos2i(-dim * asp + 0.05 * dim, dim - 0.05 * dim);
		Print("FPS=%d", windowobj->FramesPerSecond());

		get_packet();

		// check for display errors
		int err = glGetError();
		if (err) {
			fprintf(stderr, "ERROR: %s [%s]\n", gluErrorString(err), "display");
		}
		// swap buffers
		glFlush();
		glfwSwapBuffers(windowobj->glwindow);
		// get key board events
		glfwPollEvents();
	}
}

/**
 * @brief program entry point
 *
 * @param argc
 * @param argv
 * @return int
 */
int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[])
{
	Window mainwindow("Telemetry", 0, 1000, 1000, key);
	glDisable(GL_DEPTH_TEST);
	glClearColor((float)midnight.r / 255.0, (float)midnight.g / 255.0,
		     (float)midnight.b / 255.0, 1.0);

	display_loop(&mainwindow);
}
