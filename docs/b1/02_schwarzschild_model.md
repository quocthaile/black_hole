# B1 phase 2: Schwarzschild model and equation derivation

## Scope and conventions

B1 models photon propagation in the vacuum exterior of a single, non-rotating, uncharged black hole. The metric signature is `(-,+,+,+)`. Use Schwarzschild coordinates `(x^0,r,theta,phi)`, where `x^0 = c t`, and dimensionless coordinates obtained by dividing every length by a fixed reference length `L0`. Thus the dimensionless Schwarzschild radius is `rho_s = 2GM/(c^2 L0)` and `f(rho)=1-rho_s/r`. For the Sag A* scene, `L0` is the physical Schwarzschild radius, so `rho_s=1`; all camera/object/disk positions are divided by the same length. This avoids the original combination of SI-sized float positions and a shader hard-coded radius.

The integration parameter `sigma` is affine and scaled so the conserved Killing energy is `E=1`. The photon is propagated from the camera into the scene with increasing `sigma` (forward ray tracing). Rays are initialized from a static orthonormal observer at camera radius `r > rho_s`. This chart and observer are not valid at the horizon; B1 stops at the exterior horizon and does not attempt interior evolution.

## Metric, radius, units

The Schwarzschild line element in these units is

$$ds^2=-f(r)(dx^0)^2+f(r)^{-1}dr^2+r^2(d\theta^2+\sin^2\theta\,d\phi^2),\qquad f(r)=1-\frac{\rho_s}{r}.$$

The physical horizon radius is $r_s=2GM/c^2$. The project uses `G=6.67430e-11 m^3 kg^-1 s^-2` and `c=299792458 m s^-1`; conversion from host SI input is `rho = r_SI/L0` and `rho_s = r_s/L0`. `x^0` and all spatial coordinates have the same length unit. The dimensionless affine parameter is `sigma = E lambda/L0`; with `E=1`, derivatives below use this normalized affine parameter.

## Christoffel symbols and geodesics

From $\Gamma^\mu{}_{\alpha\beta}=\frac12g^{\mu\nu}(\partial_\alpha g_{\nu\beta}+\partial_\beta g_{\nu\alpha}-\partial_\nu g_{\alpha\beta})$, the nonzero independent symbols needed here are

$$\Gamma^0{}_{0r}=\Gamma^0{}_{r0}=\frac{f'}{2f},\quad
\Gamma^r{}_{00}=\frac{ff'}2,\quad \Gamma^r{}_{rr}=-\frac{f'}{2f},\quad
\Gamma^r{}_{\theta\theta}=-fr,\quad \Gamma^r{}_{\phi\phi}=-fr\sin^2\theta,$$
$$\Gamma^\theta{}_{r\theta}=\Gamma^\theta{}_{\theta r}=\Gamma^\phi{}_{r\phi}=\Gamma^\phi{}_{\phi r}=\frac1r,\quad
\Gamma^\theta{}_{\phi\phi}=-\sin\theta\cos\theta,\quad
\Gamma^\phi{}_{\theta\phi}=\Gamma^\phi{}_{\phi\theta}=\cot\theta,$$
where $f'=\rho_s/r^2$. Inserting these into $\ddot x^\mu+\Gamma^\mu{}_{\alpha\beta}\dot x^\alpha\dot x^\beta=0$ gives the usual time, radial, polar, and azimuthal geodesic equations. The full spherical-coordinate form contains `1/sin(theta)` terms and is singular on the coordinate axis.

Written explicitly, the second-order equations are

$$\ddot x^0=-\frac{f'}{f}\dot x^0\dot r,$$
$$\ddot r=-\frac{ff'}2(\dot x^0)^2+\frac{f'}{2f}\dot r^2+fr\left(\dot\theta^2+\sin^2\theta\,\dot\phi^2\right),$$
$$\ddot\theta=-\frac2r\dot r\dot\theta+\sin\theta\cos\theta\,\dot\phi^2,$$
$$\ddot\phi=-\frac2r\dot r\dot\phi-2\cot\theta\,\dot\theta\dot\phi.$$

These make the historical radial angular factor issue explicit: the angular contribution to the radial acceleration is `f r (thetaDot^2 + sin(theta)^2 phiDot^2)`, not `r (...)`.

Spherical symmetry implies every geodesic lies in a plane through the origin. Rotate axes so that the ray's orbital plane is the equator (`theta=pi/2`), then write its within-plane angle as `psi`. The planar equations are

$$\dot x^0=\frac{E}{f},\qquad \dot\psi=\frac{L}{r^2},$$
$$\ddot r=-\frac{E^2\rho_s}{2r^2f}+\frac{\rho_s\dot r^2}{2r^2f}+\frac{fL^2}{r^3},\qquad E=1.$$

This is the reduced three-variable state `(r, psi, dr/dsigma)`; `L` is a ray parameter held constant by spherical symmetry and the cyclic coordinate `psi`. The Cartesian spatial position is reconstructed as `r(cos(psi) e_r0 + sin(psi) e_t0)`, with `e_r0` the initial radial unit vector and `e_t0` the initial transverse direction in the camera ray plane. This representation supports general 3D rays without integrating polar coordinates through an axis.

Substitute the conserved `E` and `L` into the null condition to obtain the radial first integral

$$\dot r^2=E^2-f(r)\frac{L^2}{r^2}.$$

Differentiating this identity and extending continuously through radial turning points gives

$$\ddot r=\frac{L^2}{r^3}\left(1-\frac{3\rho_s}{2r}\right).$$

R0 and B1 integrate this null-constrained form, with `E=1`, because it removes a cancellation between terms proportional to `1/f` near the horizon. Numerical integration can still drift from the first integral; the null residual remains a measured diagnostic. The photon-sphere solution is `r=3 rho_s/2`, `|L/E|=3 sqrt(3) rho_s/2`.

## Null constraint and conserved quantities

The null tangent obeys

$$g_{\mu\nu}\dot x^\mu\dot x^\nu=-f\dot x^{0,2}+\frac{\dot r^2}{f}+r^2\dot\psi^2=0.$$

Stationarity and axial symmetry give $E=f\dot x^0$ and the oriented plane component $L=r^2\dot\psi$. In a general 3D orientation, the conserved angular momentum is the vector $\mathbf L=\mathbf x\times\mathbf p$; its magnitude is $|L|$ in the orbital plane and its component about a chosen global z-axis is `Lz`. R0 can preserve either sign of the plane component; the GPU chooses a tangent basis along the projected camera direction and therefore starts with nonnegative `L`. Neither implementation treats the plane magnitude as global `Lz`.

At camera radius `r0`, the local static observer measures a unit-energy spatial direction `n`. Decompose it into radial `n_r=n dot e_r0` and transverse part `n_perp=n-n_r e_r0`. The locally measured photon energy is rescaled so the conserved Killing energy is one. The resulting initial values are

$$\dot r_0=n_r,\qquad L=\pm\frac{r_0\lVert n_\perp\rVert}{\sqrt{f(r_0)}},\qquad \dot\psi_0=\frac{L}{r_0^2},\qquad \dot x^0_0=\frac1{f(r_0)}.$$

The sign of `L` records orientation relative to a caller-supplied unit tangent basis; its magnitude is the total angular momentum per unit Killing energy. R0 stores an orthonormal 3D radial/tangent basis and reconstructs the ray in that plane. The development GPU initializes the same basis from the camera position and screen direction.

Substitution gives `-E^2/f + dr^2/f + L^2/r^2 = 0` initially. At later points, the residual

$$\epsilon_{null}=\frac{| -E^2/f+\dot r^2/f+L^2/r^2 |}{E^2/f+\dot r^2/f+L^2/r^2}$$

measures integration drift. With constants used directly, `E` and `L` are conserved by construction; an independent full-state reference would be needed to measure drift of those constants from the unconstrained eight-component equations.

## Affine scale and physical limits

The local energy choice fixes an otherwise arbitrary positive affine scaling; the subsequent rescaling sets Killing `E=1`. It does not change the spacetime path or hit/miss result. `r` and `rho_s` are in the same dimensionless length units, while `E`, `L`, and `sigma` use the normalized convention above. The static-observer initialization requires `f(r0)>0`. The reduced null RHS remains finite at `f=0`, so it can locate a segment crossing of `r=rho_s`; it does not continue the physical ray into the black-hole interior. This is an exterior event surface, not horizon-penetrating interior evolution.

The model excludes spin, charge, plasma dispersion, emission/absorption, and metric backreaction. The accretion disk and luminous objects are scene event surfaces, not matter sources in the metric. A disk event is intersection with the configured plane `y=0` and radial annulus, solved linearly within the step segment for a first-order event location; an object event is a first segment/sphere intersection. Event surfaces are scene conventions. Escape is an exterior radius cutoff, not a proof of asymptotic escape.

## Corrected mapping and B0 comparison

| Mathematical quantity | Implementation mapping | Difference from historical B0 |
| --- | --- | --- |
| `r_s=2GM/c^2`, `f=1-r_s/r` | `reference::ModelParameters`; B1 shader uniform model | B0 shader hardcodes rounded SI `r_s` and uses float SI state |
| Local-frame null initialization | `reference::initializeRay`; B1 shader `initRay()` | B0 projects direction to spherical components and initializes energy with the wrong radial metric factor |
| `E=f dx^0/dsigma=1`, plane `L=r^2 dpsi/dsigma` | `Ray::energy`, `Ray::angularMomentum`; GPU ray fields | B0 `L` is `r^2 sin(theta) dphi`, with inconsistent use and convention |
| Reduced planar radial and azimuthal equations above | `reference::geodesicRHS`; B1 `geodesicRHS()` | B0 radial angular coefficient is `r` instead of `fr` and does not evolve with RK4 |
| Classical RK4, four RHS evaluations per fixed step | `reference::rk4Step`; B1 `rk4Step()` | B0 shader's `rk4Step()` is one-stage Forward Euler |
| Segment-based first event tests/status | `reference::findSegmentEvent`; B1 event functions | B0 checks only endpoint containment and an endpoint-radius disk test |

## Independent sources

1. David Tong, *General Relativity*, section 1.3.1 and 1.3.5, especially equations (1.36), (1.53)–(1.55), for the Schwarzschild metric, planar reduction, null effective potential, photon sphere, and weak-field bending: [Cambridge lecture notes](https://www.damtp.cam.ac.uk/user/tong/gr/grhtml/S1.html).
2. Matthias Blau, *Lecture Notes on General Relativity*, Part D, sections 24.4 and 25.1–25.11, for Schwarzschild coordinates/radius and particle/photon orbits: [author-hosted notes](https://blau.itp.unibe.ch/newlecturesGR.pdf).

The source list is not treated as measured validation evidence. Numerical acceptance thresholds and actual test results are recorded separately in `05_numerical_validation.md`.
