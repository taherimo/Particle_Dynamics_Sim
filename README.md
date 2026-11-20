# 🌌 Particle Dynamics Simulator

A C-based simulation project designed for undergraduate students of physics to explore **classical particle dynamics** through numerical integration.

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
├── engine/                # [Instructor Only] Core physics engine (DO NOT MODIFY)
│   ├── potential_engine.c 
│   └── potential_engine.h 
├── src/                   # [Student Workspace] Your implementation area
│   └── main.c             
├── resources/             # Simulation configuration settings
│   └── config.txt         
├── output/                # Generated trajectory and energy data
│   └── .gitkeep           
├── .gitignore             # Git exclusion rules (Ensures clean repository)
└── README.md              # Project documentation
```

> **⚠️ Important:** Do not modify any files within the `engine/` directory. Use the provided header to interact with the potential evaluator.

---

## 📝 3. Configuration & Potential Functions

### 📄 `config.txt` Format
The configuration is section-based using `name = value` syntax. Blank lines and lines starting with `#` are ignored.

**Example Configuration:**
```ini
[simulation]
mass = 1.0
time_step = 0.001
steps = 10000
output_file = output/trajectory.csv

[initial_conditions]
x0 = 1.0
y0 = 0.0
vx0 = 0.0
vy0 = 0.5

[parameters]
k = 2.0

[potential]
expression = 0.5*k*(x^2 + y^2)
```

### 🧪 Supported Math Syntax
In the `[potential]` section, you can define the energy landscape using:
* **Coordinates:** `x`, `y`
* **Operators:** `+`, `-`, `*`, `/`, `^` (exponentiation)
* **Functions:** `sin`, `cos`, `tan`, `exp`, `log`, `sqrt`
* **Constants:** `pi`, `e`
* **User Parameters:** Defined under the `[parameters]` section (e.g., `k = 2.0`).

**Examples:**
* *Harmonic Oscillator:* `0.5*k*(x^2 + y^2)`
* *Double Well:* `a*(x^2 - b^2)^2 + 0.5*k*y^2`
* *Periodic Potential:* `V0*(2 - cos(x) - cos(y))`

---

## ⚙️ 4. Numerical Integration: Velocity Verlet

For each time step $\Delta t$, you must implement:
1.  $\mathbf{a}_n = \mathbf{F}(\mathbf{r}_n) / m$
2.  $\mathbf{r}_{n+1} = \mathbf{r}_n + \mathbf{v}_n \Delta t + 0.5 \mathbf{a}_n \Delta t^2$
3.  $\mathbf{a}_{n+1} = \mathbf{F}(\mathbf{r}_{n+1}) / m$
4.  $\mathbf{v}_{n+1} = \mathbf{v}_n + 0.5(\mathbf{a}_n + \mathbf{a}_{n+1})\Delta t$

---

## 📊 5. Expected Output

The simulation must write a `.csv` file to the path specified in `config.txt`. Each step must include:
`step, time, x, y, vx, vy, kinetic_energy, potential_energy, total_energy`

**Validation Checklist:**
* [ ] Does the total energy remain approximately constant?
* [ ] Does reducing the `time_step` improve accuracy?
* [ ] Does the program handle invalid math expressions gracefully?

---

## 🚀 6. Getting Started

### 📥 6.1 Get the Project
```bash
# Recommended: Clone via Git
git clone https://github.com/YOUR_USERNAME/Particle_Dynamics_Sim.git
cd Particle_Dynamics_Sim
```

### 🛠 6.2 Compile & Run
**Using GCC/Clang (Recommended):**
Run this from the project root:
```bash
gcc -std=c11 -Wall -Wextra -Wpedantic -I./engine src/main.c engine/potential_engine.c -o particle_sim -lm
./particle_sim
```

**Using an IDE (VS Code, CLion, Code::Blocks):**
1.  Add `src/main.c` and `engine/potential_engine.c` to your project.
2.  Add `engine/` to your **Include Paths**.
3.  **CRITICAL:** Set your IDE's **Working Directory** to the `Particle_Dynamics_Sim` folder so it can find `resources/config.txt`.

---

## ✅ 7. Testing Checklist
Before submitting, ensure you have verified:
- [ ] Configuration file is read correctly.
- [ ] Potential evaluation works for various functions.
- [ ] Trajectory and energy data are saved to the correct CSV file.
- [ ] The simulation handles "invalid" math (like `log(-1)`) without crashing.

---

## 📤 8. Submission & Version Control

We follow a professional Git workflow:

1.  **Fork** the instructor's repository on GitHub.
2.  **Clone** your own fork to your computer.
3.  **Implement** your code in `src/main.c`.
4.  **Commit & Push** your changes:
    ```bash
    git add src/main.c resources/config.txt
    git commit -m "Implemented Verlet integration"
    git push origin main
    ```
5.  **Submit** the URL of your GitHub repository.
