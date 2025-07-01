# 🧱 3D Physics Simulation Engine

A modular, real-time 3D Physics Engine written in **C++** using **OpenGL**, built from scratch to simulate rigid body dynamics with realistic collisions, spatial partitioning, and multithreading. Designed to offer complete camera interactivity and controls, and scalable performance—ideal for learning and extending low-level simulation logic.

---

## 📸 Features Overview

### 🛠️ Core Implementations
- 🔷 **Rigid Body Dynamics**: Simulates Newtonian physics including linear and angular motion.
- 🧱 **Object Types**:
  - Spheres
  - Planes/Walls (static)
- 💥 **Collision Detection**:
  - Broad Phase: Optimized using **BVH (Bounding Volume Hierarchy)** and **AABB**.
  - Narrow Phase: Contact resolution using impulse-based techniques.
- 🧠 **Multithreaded Physics**:
  - Utilizes `Mutex` for parallel collision detection and integration.
  - Thread-safe force accumulation and contact manifold updates.
- ⚡ **Optimized Rendering**:
  - Vertex data stored and managed via **Vertex Buffer Objects (VBOs)** and **Vertex Array Objects (VAOs)**.
  - Efficient GPU-side buffer updates (vertexization).

---

### 🎮 User Interaction
- 📷 **Camera Control**:
  - First-person navigation (WASD + mouse).
  - Zoom and rotate using scroll and drag.
- 🖱️ To be built: **UI Interface**:
  - Build using **ImGui** for real-time property editing and visualization toggles.
  - Allowing dynamic tuning of simulation parameters.

---

## 💻 Technologies Used

| Tech              | Purpose                           |
|-------------------|-----------------------------------|
| `C++17`           | Core language and STL usage       |
| `OpenGL 4.5`      | Graphics rendering                |
| `GLFW`            | Window creation and input handling|
| `GLAD`            | OpenGL function loader            |
| `Mutex`           | Multithreading                    |

---

## 🧠 Memory & Architecture

- 💾 **Memory Management**:
  - Smart pointers (`std::unique_ptr`, `std::shared_ptr`) used for scene graph and object lifetimes.
- 🧩 **Modular Class Design**:
  - `Spheres`, `Cuboids`, `Camera`, `Shader`, `Constraint` e.t.c modules.
  - Separation of concerns maintained via clean interfaces.
- 🧮 **Mathematics**:
  - Custom math library for `Vector3`, `Quaternion`, `Matrix4`, optimized for physics.
---

## ⚙️ Optimizations

- 📦 **Spatial Partitioning**:
  - Bounding Volume Hierarchies (BVH) to reduce collision pairs.
- 🧵 **Multithreading**:
  - Physics steps (integrate, detect, resolve) parallelized using Mutex.
- 🎥 **Rendering Pipeline**:
  - GPU-driven instancing potential via glDrawElements, dynamic vertex buffers.

---

## 🔬 Sample Implemented Features

| Component               | Status    |
|-------------------------|-----------|
| Rigid Body Motion       |   ✅      |
| Sphere Collisions       |   ✅      |
| Plane Collisions        |   ✅      |
| Multithreading (Mutex)  |   ✅      |
| Spatial Partitioning    |   ✅      |
| Camera Controls         |   ✅      |
| GPU Buffer Management   |   ✅      |

---

## 📷 Screenshots and videos

![A Video of Collision with multiple objects in the Scene (3 Dimensional)](https://github.com/user-attachments/assets/53d4ab9b-5e7f-48ec-9362-96c1e14ca395)

![Sample Script Run](https://github.com/user-attachments/assets/66341ab4-bd8c-4835-8ae3-a39355c39014)

---

## 🙏 Acknowledgements

> This project draws major inspiration and learning from the following creators:

- 🎥 [Cherno](https://www.youtube.com/@TheCherno): C++ and OpenGL tutorials.
- 🧠 [AngeTheGreat](https://www.youtube.com/@AngeTheGreat): Rigid body physics and constraint solver implementations.

---

## 🏁 Getting Started

### 🧱 Requirements
- C++17 or above
- CMake 3.10+
- OpenGL drivers
- GLFW, GLAD, ImGui (included or submoduled)

### ⚙️ Build Instructions
```bash
git clone https://github.com/Nikhil-iitg27/PhysicsEngine.git
cd PhysicsEngine
mkdir build && cd build
cmake ..
make
./PhysicsEngine
