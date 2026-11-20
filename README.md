# 🌌 Particle Dynamics Simulator

A C programming project designed for undergraduate students of physics to explore **classical particle dynamics** through numerical integration.

The required project simulates **one particle moving in two-dimensional space** under a user-defined, time-independent potential energy function. The potential is entered as a mathematical expression in `resources/config.txt`; students do not need to modify the potential engine when they want to try a different supported potential.

---

## 🎯 Project Objectives

The goal of this project is to implement a particle motion simulator using the **Velocity Verlet** method. By interacting with a pre-built "Potential Engine," you will simulate how particles behave under various force fields, focusing on the core principles of **classical mechanics**.

**Key Learning Goals:**
* ✅ Translating Newton’s equations into a numerical C simulation.
* ✅ Implementing the Velocity Verlet integration algorithm.
* ✅ Managing configuration files and mathematical expressions.
* ✅ Calculating kinetic, potential, and total energy.
* ✅ Investigating numerical stability and energy conservation.
* ✅ Organizing, compiling, and submitting code via Git.

---

## 📂 2. Directory Structure

```text
Particle_Dynamics_Sim/
├── engine/                # Core physics engine (DO NOT MODIFY)
│   ├── potential_engine.c 
│   └── potential_engine.h 
├── src/                   # Your implementation area
│   └── main.c             
├── resources/             # Simulation configuration settings
│   └── config.txt         
├── output/                # Generated trajectory and energy data
│   └── .gitkeep           
├── .gitignore             # Git exclusion rules (Ensures clean repository)
└── README.md              # Project documentation
```

> **⚠️ Important:** Do not modify any files within the `engine/` directory. Use the provided header to interact with the potential evaluator.

### What is provided and what must be implemented?

The `engine/` directory contains the instructor-provided potential-expression engine. It parses the expression, evaluates the potential energy and its spatial derivatives, and returns the force. **Do not modify the files in this directory.** Use the interface declared in `potential_engine.h`.

The starter `src/main.c` provides the overall program structure. Students must complete two functions:

1. **`read_config()`**: Read the settings, initial conditions, parameters, and potential expression from `resources/config.txt`.
2. **`verlet_step()`**: Implement one step of the Velocity Verlet algorithm and evaluate the force at the updated position.

The remaining starter code initializes the potential engine, runs the simulation loop, and writes the results to a CSV file. You may add helper functions or additional source files when useful.


---

## 📝 3. Configuration & Potential Functions
The simulation settings and potential expression are read from `resources/config.txt`. The file is divided into sections. Each setting is written as `name = value`. Blank lines and lines beginning with `#` are comments and should be ignored by the configuration reader. Keep the potential expression on a single line.

### 📄 `config.txt` Format
The configuration is section-based using `name = value` syntax. Blank lines and lines starting with `#` are ignored.

**Example Configuration:**
```ini
# All physical quantities must use a consistent system of units.

[simulation]
mass = 1.0
time_step = 0.001
steps = 10000
output_file = output/trajectory.csv

[initial_conditions]
x0 = 1.0
y0 = 0.0
vx0 = 0.0
vy0 = 1.0

[parameters]
k = 2.0

[potential]
expression = 0.5*k*(x^2 + y^2)
```

The example describes a two-dimensional isotropic harmonic oscillator:

\[
U(x,y)=\frac{1}{2}k(x^2+y^2).
\]

The configuration entries have the following meanings:

| Section | Setting | Meaning |
|---|---|---|
| `[simulation]` | `mass` | Particle mass; must be positive |
| `[simulation]` | `time_step` | Fixed integration time step; must be positive |
| `[simulation]` | `steps` | Number of integration steps |
| `[simulation]` | `output_file` | Path of the CSV output file |
| `[initial_conditions]` | `x0`, `y0` | Initial position |
| `[initial_conditions]` | `vx0`, `vy0` | Initial velocity |
| `[parameters]` | User-defined names | Numerical constants used in the potential expression |
| `[potential]` | `expression` | Mathematical expression defining \(U(x,y)\) |

The variables `x` and `y` in the potential expression represent the particle's **current** position. Names declared in `[parameters]` are constant parameters. For example, `k = 2.0` allows the expression to use `k`.

Use consistent physical units throughout the configuration. The program does not perform unit conversion. The `output/` directory must exist before the program is run.


### 🧪 Supported Math Syntax

The potential engine supports the following expression syntax:

| Category | Supported syntax | Example |
|---|---|---|
| Coordinates | `x`, `y` | `x^2 + y^2` |
| Arithmetic operators | `+`, `-`, `*`, `/` | `2*x - 3*y` |
| Unary signs | Unary `+` and `-` | `-x`, `+(x+y)` |
| Exponentiation | `^` | `x^2`, `x^0.5` |
| Parentheses | `( ... )` | `(x+y)^2` |
| Trigonometric functions | `sin`, `cos`, `tan` | `sin(x)*cos(y)` |
| Exponential function | `exp` | `exp(-x^2)` |
| Logarithm | `log` | `log(x^2+y^2)` |
| Square root | `sqrt` | `sqrt(x^2+y^2)` |
| Constants | `pi`, `e` | `cos(pi*x)` |
| User parameters | Names declared in `[parameters]` | `k*x^2` |

Expression-writing rules:

- Write multiplication explicitly: use `2*x`, not `2x`.
- Use `^` for exponentiation. It does not mean bitwise XOR in this expression language.
- Exponentiation is right-associative: `x^2^3` means `x^(2^3)`.
- Exponentiation has higher precedence than unary minus: `-x^2` means `-(x^2)`. Use `(-x)^2` to square the negative quantity.
- Function names and built-in constants are lowercase and case-sensitive. Each listed function takes one argument in parentheses, such as `sin(x)`.
- Use parameter names that begin with a letter or underscore and contain only letters, digits, or underscores. Names are case-sensitive. The names `x`, `y`, `pi`, `e`, and built-in function names are reserved.
- Do not use C expressions such as `pow(x, 2)`, the `**` operator, or implicit multiplication.

Here, “user-defined” or “arbitrary” potential means any expression that can be represented using this documented syntax. The engine does not execute arbitrary C code or support every possible mathematical function.

---

## ⚙️ 4. Numerical Integration: Velocity Verlet

Implement the **Velocity Verlet** method using a fixed time step \(\Delta t\). At the beginning of a step, the particle has position \(\mathbf r_n\), velocity \(\mathbf v_n\), and acceleration \(\mathbf a_n\).

First, update the position:

\[
\mathbf r_{n+1}=\mathbf r_n+\mathbf v_n\Delta t+
\frac{1}{2}\mathbf a_n\Delta t^2.
\]

Evaluate the force at the new position using the potential engine, then calculate the new acceleration:

\[
\mathbf a_{n+1}=\frac{\mathbf F(\mathbf r_{n+1})}{m}.
\]

Finally, update the velocity:

\[
\mathbf v_{n+1}=\mathbf v_n+
\frac{1}{2}(\mathbf a_n+\mathbf a_{n+1})\Delta t.
\]

Apply these equations to both the `x` and `y` components. Do not overwrite the old acceleration before it has been used to update the velocity.

---

## 📊 5. Expected Output

The simulation must write a CSV file to the path specified by `output_file`. Include the initial state at time zero and the state after every integration step. The file should contain these columns:

```text
step,time,x,y,vx,vy,kinetic_energy,potential_energy,total_energy
```

The energies are

\[
K=\frac{1}{2}m(v_x^2+v_y^2),\qquad E=K+U(x,y).
\]

You can use the CSV file to plot the trajectory and energy profiles with Python, MATLAB, Excel, or another tool. Implementing a graphical interface in C is not required.

For a time-independent conservative potential, the exact total mechanical energy is conserved. A numerical method does not generally keep the computed energy exactly constant at every step; small fluctuations are expected. Compare results obtained with different time steps and investigate how the time step affects the trajectory and energy error.

---

## 🚀 6. Getting Started

First, fork the this repository on GitHub. Then clone **your fork** (replace `YOUR_GITHUB_USERNAME` with your GitHub username):

### 📥 6.1 Get the Project
```bash
# Recommended: Clone via Git
git clone https://github.com/YOUR_USERNAME/Particle_Dynamics_Sim.git
cd Particle_Dynamics_Sim
```

You can also download your fork as a ZIP file and extract it. Keep the directory structure intact.

### 🛠 6.2 Compile & Run

**Using GCC/Clang (Recommended):**

Compile from the project root directory:

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic -I./engine \
    src/main.c engine/potential_engine.c -o particle_sim -lm
```

Then run:

```bash
./particle_sim
```

The program expects to be run from the project root so that it can find `resources/config.txt` and the relative output path. On Windows, run the generated executable as `particle_sim.exe` (or `./particle_sim.exe`, depending on the terminal).

**Using an IDE (VS Code, CLion, Code::Blocks):**

You may use Code::Blocks, Visual Studio Code, CLion, or another C development environment.

1. Open the cloned project folder.
2. Add `src/main.c` and `engine/potential_engine.c` to the build.
3. Add `engine/` to the compiler's header/include search paths.
4. Link the mathematical library (`libm`) if required by your compiler and platform.
5. Set the program's working directory to the project root directory.

---

## ✅ 7. Testing Checklist

Before submitting, check that:

- The configuration file is read correctly, including the potential expression and all its parameters.
- The simulator works with more than one potential function.
- The initial state and every subsequent state are written to the CSV file.
- Position, velocity, and energy values are finite and physically reasonable.
- Invalid expressions or mathematical evaluations are reported without crashing.
- Total energy remains approximately constant for a time-independent conservative potential.
- Reducing the time step gives results consistent with improved numerical accuracy.

---

## 📤 8. Submission & Version Control

We follow a professional Git workflow:

1.  **Fork** this repository on GitHub.
2.  **Clone** your own fork to your computer.
3.  **Implement** your code in `src/main.c`.
4.  **Commit & Push** your changes:
    ```bash
    git add src/main.c resources/config.txt
    git commit -m "Implemented Verlet integration"
    git push origin main
    ```
5.  **Submit** the URL of your GitHub repository.
