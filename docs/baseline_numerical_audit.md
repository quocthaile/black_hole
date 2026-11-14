# Historical numerical audit (documentation only)

This supports the baseline-identification part of B-01/B-02 and preparation for
H-01/H-05 in [AGENT_REQUIREMENTS.md](AGENT_REQUIREMENTS.md), proposal pages 2, 4, 6.
It does not establish a validated numerical reference. Neither the CPU source nor
any GLSL file is modified on this branch.

## Existing source behavior

| Component | `src/baseline/CPU-geodesic.cpp` | `src/baseline/geodesic.comp` |
| --- | --- | --- |
| `Ray` | Cartesian position, spherical state and E/L stored as double; constructor inputs and camera basis use `glm::vec3` floats | Same categories of state stored as float |
| Initialization | `Ray(vec3 pos, vec3 dir)` constructor; no function literally named `initRay` | `initRay(vec3 pos, vec3 dir)` |
| Coordinates | `theta=acos(z/r)`, `phi=atan2(y,x)`; theta is measured from z | Same convention; GLSL two-argument `atan(y,x)` |
| E and L | `L=r*r*sin(theta)*dphi`; `E=f*sqrt(dr*dr/f + r*r*(dtheta*dtheta + sin(theta)^2*dphi*dphi))` | Same algebra, different precision; CPU-computed versus hardcoded Schwarzschild radius |
| `geodesicRHS` | Six double outputs; supplied `rs`; `dt/dlambda=E/f` | Two float `vec3` outputs; hardcoded `SagA_rs`; same displayed equations |
| `addState` | Six-component `out=a+factor*b` helper | No corresponding helper |
| `rk4Step` | Four RHS stages at offsets 0, h/2, h/2, h; weighted `(1,2,2,1)/6` update: classical RK4 applied to the existing RHS | One RHS evaluation followed by `state += h*RHS(state)`: Forward Euler, despite the name |
| Cartesian update | `Ray::step()` checks `r<=rs`, calls RK4, then reconstructs Cartesian coordinates | Reconstruction is inside `rk4Step()`; caller checks horizon before stepping |
| Limits | h=`1e7`, 10,000 steps, escape radius `1e14` in `raytrace()` | h=`1e7`, 60,000 steps, escape radius `1e30` in `main()` |
| Events/output | Horizon classification with a geodesic toggle and a separate straight-ray mode; CPU RGB image | Horizon, disk y-plane crossing, endpoint object containment, escape; compute RGBA image |

The GPU contains some `double` declarations (`ESCAPE_R`, disk color ratio), but
this does not make its integrated state double precision. Conversely the CPU
double state does not recover precision already lost in float camera inputs.
The CPU and GPU also differ in screen-ray construction (the sign of the up-vector
term), scene/rendering and termination limits. CPU output is not a drop-in oracle.

## Mathematical and physical concerns to validate later

- The energy initialization, affine-parameter/time units and factors of
  `f=1-r_s/r` need an independent derivation from the chosen null-geodesic model.
  A shared expression in CPU and GPU code is not evidence that it is correct.
- The radial RHS has an angular contribution `r*(dtheta^2 + sin(theta)^2*dphi^2)`.
  Its metric factors and the whole radial equation require physical validation.
- The meaning of stored `L=r^2*sin(theta)*dphi` must be established for general
  3D rays before treating it as a conserved-angular-momentum diagnostic. The RHS
  currently uses E and does not use L. Storing either value does not prove conservation.
- Division by `r`, `sin(theta)` and `f` exposes origin, polar-axis and horizon
  singularities. Intermediate CPU RK4 stages can approach invalid regions even
  when the starting state passes its horizon check. No convergence/error control
  or finite-value checks establish reliability of either integrator.
- Endpoint sphere tests and sign-change disk tests can miss or mislocate events.
  The disk test uses a y sign change and the new endpoint's x-z radius, not a
  solved crossing position. Coordinates use a z polar axis while the scene disk
  is in the x-z plane; the conventions need explicit validation together.
- CPU `SagA.r_s` is computed from mass and physical constants; the shader uses
  a rounded hardcoded radius. Matching those conventions is future work.

Related host/GPU data concerns are also preserved: the camera upload uses a C++
`bool` and uninitialized padding, while GLSL uniform blocks have layout rules;
the object upload packs scalar mass arrays densely rather than using std140's
scalar-array stride. Mass is not used by the current compute shader. These
limitations must not be silently repaired or treated as a validated data contract.

## Reusable ideas and future prerequisites

The CPU six-component state, separate RHS and four-stage structure may inform a
future independent reference implementation after the model is derived and tested.
Before that work: retain this lock and raw M0 datasets, specify coordinates/units
and E/L definitions, reconcile initial conditions and event rules, validate against
appropriate analytical cases, and plan step refinement/convergence and invariant
checks. Only then implement and test a fixed-step CPU reference and a corrected
development shader on separate branches. Do not copy the CPU RK4 into B0.

No solver, equation, step size, step limit, event test, coordinate representation,
shader precision or rendering algorithm has been corrected in M0. The historical
implementation remains available for performance comparison, not as physical truth.
