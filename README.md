# OpenGL Scene

A real-time 3D scene built with OpenGL, demonstrating a range of rendering and simulation techniques studied over the academic year. The scene depicts an interior room with dynamic lighting, animated characters, and physically-based cloth simulation.

---

## Features

### Compute Shaders
Curtains blowing in the wind are simulated using a mass-spring cloth model running entirely on the GPU via compute shaders. Each invocation updates a single vertex's position and velocity based on spring forces from neighbouring vertices, allowing the simulation to run in parallel at interactive frame rates.

### Particle Systems
A particle system simulates rain falling outside the building. Particles are spawned continuously, given random velocity and lifetime values, and are prevented from being rendered inside the building.

### Planar Reflection
A mirror placed within the scene performs a full planar reflection of the surrounding environment, rendered by drawing the scene a second time from a reflected camera position with a clipping plane to eliminate geometry below the mirror surface.

### Rigged Characters
A skeletal character automatically navigates around the room using waypoints with a full walk cycle. Bone transformations are uploaded to the GPU per frame, and vertices are blended between multiple joints using standard linear blend skinning.

### Shadow Maps
Directional shadow mapping is used to cast accurate hard shadows from a lamp in the centre of the scene. A depth map is rendered from the light's perspective and sampled during the main render pass to determine shadowed fragments.

### Attenuated Light
Point lights in the scene use quadratic attenuation, so light intensity falls off naturally with distance, producing realistic local lighting rather than uniform illumination.
