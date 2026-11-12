# Yêu cầu thực hiện khóa luận dành cho agent

> Chỉ triển khai công việc phục vụ đề cương này. Mỗi thay đổi phải truy vết được tới một yêu cầu bên dưới và một mục của đề cương. Không tự mở rộng sản phẩm, thay đổi physical model, đổi thuật toán nghiên cứu hoặc công bố kết quả chưa đo.

## 1. Nguồn, mục đích và cách sử dụng

- **Nguồn duy nhất để xác định phạm vi:** `CLEAN_VB2_DECUONGKLTN_24540024_24540035.pdf`, 7 trang, do người dùng đính kèm.
- **SHA-256 của PDF đã đối chiếu:** `bcdf5e26c837f39414d198f33e2b4942f9b4557a9fbcea7fa3ee842d94d9d5c9`.
- **Tên đề tài:** PHÁT TRIỂN PHẦN MỀM MÔ PHỎNG VẬT LÝ THIÊN VĂN VỚI CHI PHÍ TÍNH TOÁN THẤP.
- **Tên tiếng Anh:** DEVELOPMENT OF LOW-COMPUTATIONAL-COST ASTROPHYSICAL SIMULATION SOFTWARE.

Đây là bản chuyển yêu cầu của đề cương thành chỉ dẫn có thể thực hiện và kiểm tra. Tài liệu này không xác nhận tình trạng implementation của bất kỳ repository nào; chưa có source code, build hay benchmark nào được kiểm tra trong quá trình lập file này.

Các nội dung ghi **Yêu cầu đề cương** là nghĩa vụ của đề tài. Các nội dung ghi **Quy tắc làm việc cho agent** là cách kiểm soát việc thực hiện các nghĩa vụ đó, không phải yêu cầu nghiên cứu hoặc tính năng bổ sung. Nếu cách diễn giải trong file này khác PDF, dùng PDF làm căn cứ, ghi nhận khác biệt và không tự sửa phạm vi.

## 2. Mục tiêu và giới hạn bắt buộc

### 2.1. Mục tiêu nghiên cứu

**Yêu cầu:**

1. Đo lường, xác định bottleneck về thời gian thực thi trong cả hai mã nguồn gốc `gravity_sim` và `black_hole`.
2. Kiểm chứng việc xấp xỉ gravitational field trên Grid có thể giảm runtime theo số lượng vật thể so với Direct N-body trong các cấu hình benchmark xác định.
3. Kiểm chứng Dynamic Level of Detail (LOD) và Adaptive Geodesic Marching có thể giảm workload so với sampling cố định.
4. Tối ưu các phần tính toán có thể thực hiện song song trên GPU, đồng thời kiểm soát memory allocation, state transfer và overhead.
5. Đánh giá đồng thời computational cost, performance, numerical/physical accuracy và scalability; định lượng accuracy-performance trade-off.
6. Tạo phiên bản phần mềm xử lý workload lớn hơn **hoặc** giảm computational cost so với baseline trong những cấu hình đã xác định; không bỏ việc đo accuracy để đổi lấy FPS.

### 2.2. Kỳ vọng cần kiểm chứng, không phải kết quả mặc định

Kỳ vọng giải phóng **50%-80% tài nguyên CPU bị lãng phí**, tăng khả năng duy trì **trên 60 FPS** với **10^4 đến 10^6 vật thể/tia sáng**, và tạo tiền đề mở rộng tới hàng triệu vật thể/tia sáng.

Agent phải giữ nguyên tính chất **kỳ vọng/giả thuyết** của các con số này. Chỉ kết luận mức cải thiện runtime, CPU utilization và FPS từ số liệu thực nghiệm. Không diễn giải thành bảo đảm đạt 60 FPS ở mọi workload hoặc mọi phần cứng; không báo đạt mục tiêu khi chưa có dữ liệu. Cụm “tài nguyên CPU bị lãng phí” chưa có định nghĩa đo cụ thể trong đề cương: phải xác định từ profiling trước khi tính tỷ lệ.

### 2.3. Phạm vi được phép

**Yêu cầu:**

- Phần mềm C++ mô phỏng vật lý thiên văn trên máy tính cá nhân chạy **Windows** hoặc linux, dùng CPU/GPU tiêu chuẩn.
- Hai pipeline: **GravitySim** và **Black Hole**.
- Đối tượng nghiên cứu: lực hấp dẫn Newton, độ võng không thời gian, geodesic raytracing và các hàm API đồ họa.
- Nền tảng vật lý theo đề cương: bán kính Schwarzschild và physical model của baseline; không xây dựng lại phần mềm từ các nền tảng lý thuyết vật lý hiện đại.
- Tối ưu dựa trên mã nguồn của **kavan010/Kavan**, giữ khả năng đối chiếu với baseline.
- Tích hợp hai module vào **engine thống nhất** để thực hiện và trình diễn kết quả nghiên cứu.

### 2.4. Ranh giới chống mở rộng phạm vi

**Quy tắc làm việc cho agent:**

- Không tự bổ sung physical model, đối tượng nghiên cứu hoặc pipeline thứ ba.
- Không thay Mass Accumulation, Gaussian Blur, Dynamic Grid/Field LOD hoặc Adaptive Geodesic Marching bằng một hướng nghiên cứu khác rồi coi như đã đáp ứng đề cương.
- Không tự thêm Barnes-Hut, Gaussian Splatting hoặc polarized radiative transfer chỉ vì tên kỹ thuật xuất hiện trong tài liệu tham khảo.
- Không tự đặt yêu cầu giao diện kiểu phần mềm benchmark, hệ thống phím tắt, dashboard, website, cloud, distributed computing hoặc hỗ trợ hệ điều hành khác. Đề cương chỉ yêu cầu tích hợp engine và demo; phần giao diện cần thiết phải phục vụ trực tiếp hai việc đó.
- Không refactor toàn bộ architecture, đổi tên hàng loạt API/class/function/variable hoặc thêm framework nếu không cần cho yêu cầu đang thực hiện. Ưu tiên thay đổi tăng dần trên cấu trúc hiện hữu.
- Không nhập yêu cầu từ lịch sử trao đổi, kế hoạch khác hoặc sở thích suy đoán vào phạm vi. Yêu cầu bổ sung rõ ràng của người dùng phải được ghi riêng với ảnh hưởng tới phạm vi và benchmark.

## 3. Baseline và điều kiện thực nghiệm

### B-01. Khóa mã nguồn gốc

**Yêu cầu:**

- `black_hole`: <https://github.com/kavan010/black_hole>, tài liệu tham khảo [5].
- `gravity_sim`: <https://github.com/kavan010/gravity_sim>, tài liệu tham khảo [6].
- Ngày truy cập: 20/09/2026. Ngày này không phải commit SHA và không xác định duy nhất phiên bản source.
- Khóa phiên bản baseline của cả hai repository trước khi so sánh.
- Không thay đổi code baseline trong quá trình benchmark chính thức.

**Bằng chứng hoàn thành:** xác định được repository, commit SHA, cấu hình build và cách chạy baseline. Nếu người dùng giao một fork hoặc working branch, phải xác định quan hệ với source gốc và baseline đã khóa; không tự coi HEAD của fork là baseline gốc.

### B-02. Thiết lập reference cho từng pipeline

**Yêu cầu:**

- GravitySim: xây dựng **Direct N-body baseline/reference** và test reference.
- Black Hole: xác định, hoàn thiện **fixed-step geodesic baseline/reference**.
- Đối chiếu Black Hole với analytical reference hoặc numerical reference phù hợp để kiểm soát sai số.
- Thực hiện convergence study để đánh giá reference; không mặc định một implementation là chính xác chỉ vì được gọi là reference.

Phân biệt baseline dùng so sánh performance với reference dùng đánh giá accuracy. Nếu cùng một implementation đảm nhiệm cả hai vai trò, ghi rõ cấu hình và bằng chứng phù hợp. Không tự giả định fixed-step mặc nhiên là RK4 nếu source chưa xác nhận.

### B-03. Cố định điều kiện so sánh

**Yêu cầu:**

- Thiết lập bộ initial conditions và simulation parameters cố định.
- Xác định hardware/software configuration dùng xuyên suốt benchmark.
- Chốt experimental configurations trước khi chạy benchmark chính thức.
- So sánh hai phiên bản trong điều kiện tương đương; với ablation, chỉ thay đổi yếu tố cần khảo sát và ghi rõ thay đổi đó.
- Version bộ dữ liệu benchmark và cấu hình để có thể tái lập kết quả.

**Quy tắc ghi nhận tối thiểu:** repository/commit, build mode và compiler, Windows, CPU, GPU và driver, graphics API, cấu hình workload, tham số mô phỏng, initial conditions, cấu hình thuật toán và cách đo. Nếu dùng yếu tố ngẫu nhiên, lưu seed hoặc dữ liệu khởi tạo tương ứng. Đề cương chưa chỉ định model phần cứng, phiên bản công cụ hoặc bộ dữ liệu số cụ thể; không tự ghi chúng là yêu cầu gốc.

## 4. Profiling và tối ưu hạ tầng thực thi

### P-01. Xác định bottleneck bằng đo lường

**Yêu cầu:**

- Sử dụng **Visual Studio Performance Profiler**, đặc biệt CPU Usage, để định vị các hàm chiếm tài nguyên lớn.
- Profiling CPU/GPU, memory và frame time của cả hai pipeline.
- Phân tích runtime/frame time theo từng stage, không chỉ đo FPS toàn ứng dụng.
- Xây dựng profiling report và complexity model theo workload.

Những vấn đề allocation liên tục, memory fragmentation, CPU bottleneck và overhead API được nêu trong tổng quan là cơ sở khảo sát. Agent phải kiểm tra lại trên baseline đã khóa, không coi mô tả tổng quan là bằng chứng đo mới của phiên bản hiện tại.

### P-02. Memory, state và GPU execution path

**Yêu cầu:**

- Loại bỏ allocation và I/O không cần thiết trong frame loop.
- Hoàn thiện **Object State Management** và **GPU buffer management**.
- Chuẩn hóa **SSBO/data layout**.
- Hoàn thiện **Compute Shader execution path**.
- Chuyển các phần có thể song song như tạo đỉnh lưới, đường lưới và tương tác vật thể với lưới sang GPU.
- Kiểm soát state transfer và memory allocation; có benchmark overhead riêng.
- Bảo đảm implementation optimization không làm thay đổi physical model của baseline.

**Quy tắc làm việc cho agent:** trước khi sửa, mô tả data flow và ownership/lifetime hiện tại từ source thật. Khi thay đổi CPU/GPU data path, kiểm tra layout, phạm vi truy cập, resource lifetime và synchronization cần thiết theo API đang dùng. Không tự đặt tên class, buffer role hoặc API chưa tồn tại rồi mô tả như implementation hiện có.

**Bằng chứng hoàn thành:** GPU execution infrastructure hoạt động, có kiểm tra tính đúng đắn của state/data path và số đo overhead; nêu rõ phần nào đã chuyển sang GPU và phần nào còn trên CPU.

### P-03. OpenGL và Vulkan

**Yêu cầu:**

- Phần phương pháp yêu cầu **chuyển đổi OpenGL sang Vulkan**.
- Giai đoạn tối ưu sơ bộ yêu cầu đánh giá **OpenGL Compute Shader và Vulkan** như implementation layer.

Phải giữ **cả nhiệm vụ đánh giá và nhiệm vụ chuyển đổi** trong kế hoạch; không tự hạ yêu cầu Vulkan thành tùy chọn hoặc đánh dấu chuyển đổi hoàn tất chỉ bằng tài liệu đánh giá. Lập các thay đổi cần thiết dựa trên source hiện hữu, tách ảnh hưởng của implementation layer khỏi ảnh hưởng của thuật toán khi phân tích kết quả.

Đề cương đặt nội dung chuyển đổi dưới mục “Tối ưu hóa ở cấp độ biên dịch”. Khi triển khai, xác định đúng đây là thay đổi graphics API/implementation layer; không coi đổi compiler flag là đã thay OpenGL bằng Vulkan. Mọi kết luận Vulkan cải thiện hiệu năng phải có phép đo, không suy ra trực tiếp từ việc đổi API.

## 5. Pipeline GravitySim

**Nguồn yêu cầu chính:**

### G-01. Direct N-body

- Implement hoặc hoàn thiện Direct N-body baseline và test reference theo source được giao.
- Giữ cấu hình reference có thể chạy độc lập để đối chiếu acceleration/force, gravitational field và trạng thái vật thể.
- Không sửa reference để làm giảm sai khác với phiên bản optimized.

### G-02. Mass Accumulation trên Grid

- Implement **Mass Accumulation trên Grid**.
- Xác định từ implementation cách object-to-cell mapping, tích lũy khối lượng và sử dụng dữ liệu Grid cho gravitational field/N-body integration.
- Đo runtime stage và sai số so với Direct N-body reference.
- Phân tích chi phí theo implementation thực tế; chỉ dùng dự kiến `O(N)` cho stage accumulation khi điều kiện chi phí mapping `O(1)` được đáp ứng.

### G-03. Gaussian Blur trên gravitational field

- Implement **Gaussian Blur Pass trên gravitational field**, theo các chiều Grid được xác định.
- Giữ yêu cầu Gaussian Blur **theo hai chiều** ở phần phương pháp của đề cương; không tự đổi sang một bài toán blur khác.
- Ghi kernel size và các tham số blur sử dụng trong cấu hình thực nghiệm.
- Đánh giá riêng ảnh hưởng của blur đến computational cost và sai số field/acceleration/trạng thái vật thể.

Đề cương chưa cung cấp kernel, tham số Gaussian hoặc chi tiết boundary handling. Xác định các chi tiết này từ source và thiết kế cần thiết, ghi rõ là lựa chọn implementation; không trình bày như thông số đã được đề cương quy định.

### G-04. Dynamic Grid/Field LOD

- Implement **Dynamic Grid/Field LOD theo spatial resolution**, với lựa chọn mức chi tiết gắn với camera như phần phương pháp.
- Xác định resolution và số Grid cells thực sự sử dụng ở mỗi mức LOD.
- Đánh giá riêng mức giảm workload và spatial approximation error do LOD.
- Không coi giảm độ chi tiết phần vẽ lưới là đã đáp ứng Grid/Field LOD nếu workload gravitational field không thay đổi tương ứng.

### G-05. Grid và N-body integration

- Tối ưu **CreateGridVertices** và **Grid update path**.
- Hoàn thiện N-body integration trong pipeline có Mass Accumulation/Grid/Field LOD.
- Phân tích complexity và runtime từng stage.
- Phân biệt tác động của thay đổi field approximation với tác động của temporal discretization.

### G-06. Numerical/physical validation

Đo các nhóm sai số sau so với Direct N-body reference:

| Nhóm | Chỉ số bắt buộc theo đề cương |
| --- | --- |
| Tương tác hấp dẫn | Acceleration/force error |
| Trường | Gravitational field error |
| Trạng thái vật thể | Position error; velocity error |
| Bảo toàn | Total-energy drift |

- Thực hiện **timestep convergence study** cho N-body để đánh giá temporal discretization.
- Thực hiện **grid/field convergence study**, đối chiếu reference solution để định lượng spatial approximation error của Grid và Gaussian Blur.
- Ghi định nghĩa sai số, đơn vị hoặc chuẩn hóa, thời điểm/khoảng mô phỏng được so sánh và cấu hình reference.
- Không tự đặt một ngưỡng sai số rồi coi đó là ngưỡng chấp nhận đã có trong đề cương.

### G-07. Ablation bắt buộc

| Cấu hình | Thành phần |
| --- | --- |
| Gravity-0 | Direct N-body |
| Gravity-1 | Mass Accumulation |
| Gravity-2 | Mass Accumulation + Gaussian Blur |
| Gravity-3 | Mass Accumulation + Gaussian Blur + Dynamic Grid/Field LOD |

Đây là nhãn cấu hình thực nghiệm trong tài liệu này, không phải tên API hoặc module có sẵn. Với mỗi cấu hình, đo computational cost và accuracy tương ứng; có phân tích riêng đóng góp của Dynamic LOD.

## 6. Pipeline Black Hole

**Nguồn yêu cầu chính: trang 2 và trang 6; baseline ở trang 4.**

### H-01. Fixed-step geodesic reference

- Hoàn thiện fixed-step geodesic baseline/reference.
- Ghi initial conditions của ray, simulation parameters và cấu hình tích phân.
- Xác định analytical hoặc numerical reference phù hợp cho từng nhóm kiểm tra.
- Thực hiện convergence study cho geodesic reference.

### H-02. Adaptive Geodesic Marching

- Implement **Adaptive Geodesic Marching** với **local error estimation** và **tolerance control**.
- Sử dụng cơ sở **embedded Runge-Kutta local error estimation** theo đề cương.
- Chuẩn hóa local error theo **absolute tolerance** và **relative tolerance**.
- Dùng local error để quyết định **chấp nhận/từ chối bước** và điều chỉnh **affine-parameter step size**.
- Đo số integration steps và **RHS evaluations** thực sự thực hiện, kể cả chi phí của bước bị từ chối nếu có.
- Không coi heuristic đổi step size mà không có embedded error estimation/tolerance control là đã đáp ứng yêu cầu này.

Đề cương chưa chỉ định embedded Runge-Kutta pair, tolerance cụ thể hoặc công thức cập nhật step size. Agent phải ghi lựa chọn và căn cứ numerical analysis; không tự nói đề cương bắt buộc một solver cụ thể. Không đồng nhất số RHS evaluations của embedded solver với 4 evaluations của classical RK4.

### H-03. Early Termination và event detection

- Implement **Early Termination** và **event detection**.
- Khảo sát và xử lý các stage liên quan **event-horizon test**, **accretion-disk intersection** và **object intersection** trong pipeline theo physical model của baseline.
- Đánh giá riêng tác động của Early Termination đến workload/runtime và kết quả hit/miss.
- Kiểm tra việc dừng sớm hoặc thay đổi step size không bỏ qua event cần phát hiện; cách xử lý cụ thể phải dựa trên source và reference, không tự thay physical model.

### H-04. Dynamic Ray/Resolution LOD

- Implement **Dynamic Ray/Resolution LOD** cho workload rendering/ray sampling, theo camera.
- Ghi resolution, số ray và sampling thực tế ở các mức LOD.
- Đánh giá riêng mức giảm workload và sai số do giảm spatial/sampling resolution.
- Không chỉ giảm kích thước ảnh hiển thị rồi coi như đã giảm ray workload nếu số ray thực hiện không đổi.

### H-05. Numerical/physical validation

| Nhóm | Chỉ số bắt buộc theo đề cương |
| --- | --- |
| Quỹ đạo | Trajectory error |
| Đại lượng bảo toàn | Conserved energy error; conserved angular momentum error |
| Bẻ cong ánh sáng | Deflection-angle error |
| Sự kiện/giao cắt | Ray hit/miss classification |

- Đối chiếu analytical hoặc numerical reference phù hợp; ghi cách đối chiếu và cấu hình reference.
- Đánh giá ảnh hưởng của tolerance và geodesic reference qua convergence study.
- Báo riêng sai số do adaptive integration và do LOD/sampling, không quy mọi sai khác về cùng một nguyên nhân.

### H-06. Ablation bắt buộc

- So sánh **fixed-step geodesic**, **Adaptive Geodesic Marching**, **Dynamic LOD** và các tổ hợp tương ứng.
- Đánh giá **Adaptive**, **Early Termination** và **LOD** riêng biệt theo giai đoạn 5 của kế hoạch.
- Mỗi cấu hình phải ghi rõ kỹ thuật bật/tắt, số ray/step/RHS evaluations, runtime và numerical/physical error.

Đề cương không liệt kê sẵn toàn bộ tổ hợp bật/tắt. Thiết kế ma trận đủ tách đóng góp của ba kỹ thuật và đánh giá các tổ hợp đang dùng; không chỉ so một bản fixed-step với một bản bật tất cả rồi kết luận đóng góp của từng kỹ thuật.

## 7. Phân tích độ phức tạp

**Yêu cầu:** phân tích riêng từng stage của hai pipeline và đối chiếu mô hình với runtime thực nghiệm.

### 7.1. Ký hiệu

| Ký hiệu | Ý nghĩa trong mô hình workload |
| --- | --- |
| `N` | Số vật thể của GravitySim |
| `G` | Số Grid cells được xử lý ở resolution đang xét |
| `K` | Gaussian Blur kernel size theo quy ước implementation phải được ghi rõ |
| `P = W × H` | Số ray theo kích thước ray/sampling resolution của cấu hình |
| `S` | Số integration steps trên một ray trong mô hình; adaptive cần ghi số thực tế và cách tổng hợp |
| `M` | Số đối tượng xét trong phần intersection của Black Hole |

### 7.2. Mô hình phải phân tích và kiểm chứng

| Pipeline/stage | Mô hình được nêu trong đề cương | Điều kiện và cách diễn giải cần giữ |
| --- | --- | --- |
| Gravity: Direct N-body | `O(N²)` | Xác định phần tương tác trực tiếp trong implementation. |
| Gravity: Mass Accumulation | Dự kiến `O(N)` | Chỉ với object-to-cell mapping chi phí `O(1)`; tính thêm các stage khác nếu có. |
| Gravity: Gaussian Blur theo hai chiều | `O(GK)` | Đối chiếu cách định nghĩa kernel và implementation; không áp công thức nếu cấu trúc vòng lặp không tương ứng. |
| Gravity: LOD | `O(N)` được nêu trong đề cương | Phân tích thêm phụ thuộc resolution và số Grid cells thực tế; không suy ra toàn pipeline là `O(N)`. |
| Black Hole: ray initialization | `O(P)` | Gắn với ray resolution thực tế của cấu hình. |
| Black Hole: numerical geodesic integration | `O(PS)` | Adaptive làm thay đổi số bước thực tế; cần đo workload. |
| Black Hole: RK4 evaluation | `O(4PS)` | Hệ số 4 biểu thị số RHS evaluations của classical RK4; về bậc tiệm cận vẫn là `O(PS)`. |
| Black Hole: event-horizon/disk/object tests | Phân tích theo `P`, `S`, `M` | Tách chi phí theo stage và cách tổ chức test thực tế. |
| Black Hole: Adaptive Geodesic Marching | `O(PSM)` được nêu trong đề cương | Kiểm tra giả định về intersection/object loop; không mặc định mọi stage đều có hệ số `M`. |

**Quy tắc làm việc cho agent:** bảng này giữ mô hình của đề cương và yêu cầu phân tích theo implementation; không phải xác nhận các công thức đúng cho mọi implementation. Nếu source không phù hợp mô hình dự kiến, trình bày giả định, mô hình thực tế và dữ liệu đối chiếu. Không sửa công thức âm thầm, không suy ra speedup chỉ từ Big-O và không tuyên bố adaptive làm đổi bậc phức tạp khi chưa có cơ sở.

## 8. Benchmark, ablation, convergence và scalability

### E-01. Bộ metric bắt buộc

**Yêu cầu:**

| Nhóm | Dữ liệu cần có |
| --- | --- |
| Frame performance | Frame Time, FPS |
| Stage cost | Runtime/frame time từng stage; CPU/GPU execution time |
| Utilization | CPU utilization; GPU utilization |
| Memory | RAM; VRAM |
| Gravity workload | `N`, `G`, resolution, cấu hình LOD/blur tương ứng |
| Black Hole workload | `P`, `S`, `M`, integration steps và RHS evaluations thực tế |
| Accuracy | Toàn bộ metric GravitySim ở G-06 và Black Hole ở H-05 |
| Scalability | Xu hướng runtime và memory theo nhiều mức workload |

CPU/GPU utilization giảm không tự chứng minh thời gian chạy giảm. Báo các metric cùng nhau và gắn kết luận với workload/cấu hình cụ thể.

### E-02. Cách thực hiện

1. Khóa baseline, reference, initial conditions và cấu hình phần cứng/phần mềm.
2. Đo baseline, kiểm tra reference và ghi bottleneck ban đầu.
3. Đo overhead của implementation/data path để phân biệt với thay đổi thuật toán.
4. Chạy ablation Gravity theo G-07 và Black Hole theo H-06.
5. Chạy scalability theo `(N, G, P, S, M)` ở các mức phù hợp từng pipeline, không nhầm vật thể với ray.
6. Thực hiện timestep convergence cho N-body; grid/field convergence cho Grid và Gaussian Blur; convergence cho geodesic reference và kiểm soát sai số adaptive.
7. Phân tích statistical analysis, xu hướng runtime/memory và accuracy-performance trade-off.
8. Tổng hợp benchmark dataset, bảng/biểu đồ và kết luận theo từng hypothesis.
9. Kiểm tra reproducibility trước khi hoàn tất báo cáo và demo.

**Quy tắc làm việc cho agent:** lưu dữ liệu đo gốc, cấu hình, đơn vị, cách lấy mẫu và cách tổng hợp. Ghi quy trình warm-up/lặp đo nếu sử dụng và cơ sở lựa chọn; đề cương không ấn định số lần lặp hay phương pháp thống kê cụ thể. Với cấu hình LOD khác resolution, mô tả cách đối chiếu accuracy và báo workload thực tế để tránh so sánh thiếu tương đương.

Không ngoại suy kết quả reference ở workload nhỏ thành xác nhận accuracy/performance ở workload lớn chưa chạy. Nếu thiếu tài nguyên, timeout, lỗi hoặc công cụ chưa đo được một metric, báo rõ trường hợp và dữ liệu còn thiếu; không thay bằng giá trị ước đoán như số đo.

### E-03. Kết luận được phép

- Kết luận giảm computational cost hoặc tăng workload phải có baseline đối chiếu trong cấu hình xác định.
- Kết luận về accuracy phải kèm reference, định nghĩa metric và dữ liệu tương ứng.
- Kết luận đóng góp của từng kỹ thuật phải dựa trên ablation có thể tách tác động kỹ thuật đó.
- Kết luận scalability phải dựa trên nhiều mức workload và nêu phạm vi đã đo.
- Nếu hypothesis không được xác nhận hoặc tối ưu gây sai số lớn, báo đúng kết quả và trade-off; không che giấu dữ liệu để đạt kỳ vọng.

## 9. Tích hợp hai pipeline

**Yêu cầu:**

- Tích hợp GravitySim và Black Hole vào engine thống nhất.
- Hoàn thiện GPU resource integration, bảo đảm các cấu hình benchmark/ablation vẫn chạy được.
- Có source code demo và nội dung trình diễn phục vụ bảo vệ khóa luận.
- Giữ khả năng đối chiếu kết quả từng pipeline với baseline/reference sau khi tích hợp.

**Quy tắc làm việc cho agent:** kiểm tra kiến trúc và resource lifetime hiện tại trước khi sửa; chọn phần thay đổi tối thiểu cần thiết cho việc tích hợp. Đề cương không chỉ định cơ chế đổi pipeline, tên interface, class hierarchy hoặc thiết kế UI. Không biến các lựa chọn đó thành một dự án architecture/product riêng.

## 10. Kế hoạch và phân công theo đề cương

Các mốc sau là kế hoạch gốc, không phải xác nhận công việc đã hoàn thành. Không tự đổi ngày hoặc đánh dấu hoàn tất theo thời gian hiện tại.

| Giai đoạn | Thời gian | Công việc và đầu ra phải giữ | Phân công theo đề cương |
| --- | --- | --- | --- |
| 1. Đăng ký KLTN | 09/2026 | Hoàn thiện đề cương/phạm vi; xác định hai pipeline, research questions, baseline, performance/error metrics, ablation, scalability và hardware/software configuration. Đầu ra: hồ sơ đăng ký, research questions và experimental methodology. | Cả hai: tìm hiểu đề tài; tổng hợp thông tin, số liệu, kỹ thuật. |
| 2. Khảo sát source, baseline, profiling | 09/2026 | Khóa phiên bản hai source; Direct N-body và fixed-step geodesic reference; initial conditions/parameters cố định; CPU/GPU/memory/frame-time profiling; complexity theo `(N,G,P,S,M)`. Đầu ra: baseline implementations, profiling report, complexity model, benchmark dataset/version cố định. | Lê Quốc Thái: `gravity_sim`, Direct N-body, Grid. Kỹ Trần Thái Uyên: `black_hole`, fixed-step geodesic, ray workload. |
| 3. Tối ưu sơ bộ | 10/2026 | Allocation/I/O trong frame loop; Object State Management; GPU buffer management; SSBO/data layout; Compute Shader execution path; đánh giá OpenGL Compute Shader/Vulkan; giữ physical model; thiết kế benchmark. Đầu ra: GPU execution infrastructure ổn định, kiểm soát state transfer/allocation và overhead benchmark riêng. | Lê Quốc Thái: memory/GPU data path, SSBO, GPU memory manager. Kỹ Trần Thái Uyên: Compute Shader execution path, validation, integration, thiết kế benchmark. |
| 4. Tối ưu GravitySim | 10/2026 | G-01 đến G-07: Direct, Mass Accumulation, Blur, Grid/Field LOD, CreateGridVertices/Grid update, N-body integration, complexity/runtime và accuracy. Đầu ra: module tối ưu, ablation chạy được, dữ liệu performance/accuracy sơ bộ. | Lê Quốc Thái: Mass Accumulation, Grid/Field LOD, N-body integration. Kỹ Trần Thái Uyên: Gaussian Blur, Compute Shader, accuracy validation. |
| 5. Tối ưu Black Hole | 11/2026 | H-01 đến H-06: reference, Adaptive Geodesic Marching/error control, Early Termination/event detection, Ray/Resolution LOD, ablation riêng, numerical validation, step/RHS counts. Đầu ra: module tối ưu, adaptive solver/LOD hoạt động, dữ liệu accuracy/workload. | Lê Quốc Thái: Dynamic LOD, GPU resource integration. Kỹ Trần Thái Uyên: Adaptive Geodesic Marching, error control, Early Termination. |
| 6. Tích hợp, Ablation, Accuracy, Scalability | 12/2026 | Engine thống nhất; khóa experimental configurations/baseline; ablation, scalability, đủ metrics, convergence, trade-off, dataset và biểu đồ. Đầu ra: benchmark hoàn chỉnh, bảng/biểu đồ performance/complexity/error/ablation/scalability và kết luận theo hypothesis. | Cả hai: integration, benchmark, ablation, scalability, statistical analysis. |
| 7. Báo cáo và bảo vệ | 01/2027 | Đối chiếu hypothesis; bảng baseline/optimized; báo cáo; hình architecture/flow/algorithm/complexity/experimental results; demo, slide; kiểm tra reproducibility. Đầu ra: báo cáo KLTN, source code demo, benchmark dataset, slide và nội dung bảo vệ. | Cả hai: tổng hợp kết quả, số liệu benchmark, viết báo cáo. |

Phân công trên là trách nhiệm theo đề cương. Agent hỗ trợ theo phần việc được người dùng giao; không tự gán trạng thái đã làm cho một thành viên hoặc tự sửa phân công.

## 11. Sản phẩm cuối cùng và tiêu chí kiểm tra

### D-01. Phần mềm

- Hai pipeline GravitySim và Black Hole được tối ưu và tích hợp vào engine thống nhất.
- Có bằng chứng xử lý workload lớn hơn hoặc giảm computational cost so với baseline trong cấu hình benchmark xác định.
- Có định lượng accuracy-performance trade-off, numerical/physical error và scalability.
- Các cấu hình baseline/reference và ablation cần cho nghiên cứu có thể chạy/đối chiếu.
- Có source code demo; ghi nhận cách build/run theo môi trường Windows đã chốt.

### D-02. Báo cáo và benchmark dataset

- Quyển báo cáo khóa luận tốt nghiệp chi tiết.
- Benchmark dataset thực nghiệm so sánh trước/sau tối ưu, có cấu hình và nguồn dữ liệu đo để tái lập.
- Profiling report, complexity model, bảng baseline/optimized, ablation, convergence, accuracy và scalability.
- Hình architecture, flow, algorithm, complexity và experimental results.
- Phân tích theo hypothesis, nêu giới hạn và trade-off dựa trên số đo.
- Slide, demo và nội dung bảo vệ.

### D-03. Sản phẩm khoa học

**Yêu cầu:** hoàn thiện và gửi đăng **01 bài báo khoa học** tại hội nghị quốc tế hoặc tạp chí chuyên ngành thuộc hệ thống trích dẫn **Scopus hoặc SCIE**.

Giữ yêu cầu này trong danh sách sản phẩm, dù bảng tiến độ không có một giai đoạn gửi bài riêng. Phân biệt bản thảo, đã gửi, được chấp nhận và đã xuất bản; đề cương yêu cầu hoàn thiện/gửi đăng, không bảo đảm được chấp nhận. Agent chỉ ghi “đã gửi” khi có bằng chứng; thao tác gửi bài ra bên ngoài chỉ thực hiện khi được người dùng giao rõ ràng.

### 11.1. Checklist nghiệm thu tổng thể

- [ ] Baseline của hai source đã khóa, xác định được provenance và commit.
- [ ] Direct N-body reference và fixed-step geodesic reference có kiểm tra phù hợp.
- [ ] Có profiling CPU/GPU/memory/frame time và bottleneck theo stage.
- [ ] Có Object State Management, GPU buffer management, SSBO/data layout và Compute Shader execution path đã kiểm tra.
- [ ] Có dữ liệu overhead cho implementation/data path; physical model baseline được bảo toàn trong tối ưu hạ tầng.
- [ ] Đã đánh giá OpenGL Compute Shader/Vulkan và thực hiện yêu cầu chuyển đổi OpenGL sang Vulkan; chưa xong thì ghi rõ phần còn thiếu.
- [ ] GravitySim có Direct, Mass Accumulation, Gaussian Blur, Dynamic Grid/Field LOD, Grid path và N-body integration tương ứng.
- [ ] Black Hole có fixed-step reference, embedded adaptive error control, Early Termination/event detection và Dynamic Ray/Resolution LOD.
- [ ] Đủ các metric numerical/physical error của cả hai pipeline.
- [ ] Có timestep, grid/field và geodesic convergence studies phù hợp.
- [ ] Có ablation từng kỹ thuật và các tổ hợp cần thiết; LOD được đánh giá riêng.
- [ ] Có complexity model theo implementation và scalability theo `(N,G,P,S,M)`.
- [ ] Có engine thống nhất và demo hoạt động.
- [ ] Có benchmark dataset tái lập được, dữ liệu gốc, cấu hình, bảng/biểu đồ và kết luận dựa trên thực nghiệm.
- [ ] Có báo cáo KLTN, hình minh họa nghiên cứu, slide và nội dung bảo vệ.
- [ ] Có 01 bài báo hoàn thiện và bằng chứng gửi đăng đúng mục tiêu của đề cương.

Không đánh dấu mục đã hoàn thành chỉ vì có file, tên function, UI toggle hoặc kế hoạch. Mỗi mục cần bằng chứng implementation/validation/measurement tương ứng.

## 12. Quy trình bắt buộc cho agent khi nhận công việc

**Đây là quy tắc làm việc để bám đề cương, không phải phần mở rộng nghiên cứu.**

1. **Xác định yêu cầu:** ghi ID trong file này và trang/mục đề cương mà công việc phục vụ.
2. **Khảo sát trạng thái hiện tại:** đọc source/build/configuration liên quan; mô tả data flow, implementation đã có và phần còn thiếu. Không suy luận từ tên branch hoặc kế hoạch cũ.
3. **Chốt thay đổi nhỏ nhất cần thiết:** nêu mục đích, file/module bị ảnh hưởng, baseline/reference dùng kiểm tra và dữ liệu cần đo. Không tự tạo kiến trúc mới ngoài nhu cầu.
4. **Thực hiện và kiểm tra:** giữ terminology/API hiện hữu; kiểm tra correctness trước khi kết luận performance; giữ cấu hình ablation có thể đối chiếu.
5. **Ghi bằng chứng:** commit hoặc file/function thực sự thay đổi, cấu hình, lệnh build/run/test đã dùng, log/dataset thực tế và kết quả.
6. **Đối chiếu phạm vi:** bỏ các thay đổi không có quan hệ trực tiếp với yêu cầu; nếu thiếu thông số, ghi rõ lựa chọn implementation hoặc câu hỏi cần giải quyết.
7. **Báo trạng thái trung thực:** phân biệt implemented, verified, measured và phần chưa làm. Không biến hướng đề xuất thành tính năng đã có.

### 12.1. Trạng thái phải dùng đúng nghĩa

| Trạng thái | Ý nghĩa |
| --- | --- |
| `planned` | Đã xác định cần làm, chưa xác nhận có implementation. |
| `proposed` | Phương án đang đề xuất, chưa phải quyết định hoặc kết quả. |
| `implemented` | Đã có thay đổi source cụ thể; chưa tự suy ra đã đúng hoặc nhanh hơn. |
| `verified` | Đã kiểm tra bằng quy trình/reference cụ thể, trong phạm vi cấu hình đã nêu. |
| `measured` | Có số đo thực nghiệm và cấu hình/log/dataset tương ứng. |
| `blocked` | Chưa thể tiếp tục một phần việc; nêu nguyên nhân và bằng chứng còn thiếu. |

Một hạng mục có thể đồng thời `implemented`, `verified` và `measured` nếu có đủ bằng chứng; không dùng các từ này thay thế lẫn nhau.

### 12.2. Xử lý chi tiết đề cương chưa quy định

Các nội dung chưa được chốt trong PDF gồm commit baseline, hardware/software version cụ thể, số mức workload, initial conditions số, định nghĩa/chuẩn tổng hợp error, ngưỡng accuracy, kernel/blur parameters, LOD thresholds, embedded solver pair, tolerance và quy trình lặp đo/thống kê.

- Trước tiên tìm trong source/configuration/tài liệu dự án được người dùng giao.
- Với lựa chọn implementation thông thường trong phạm vi, chọn phương án có căn cứ, ghi giả định và ảnh hưởng để có thể kiểm tra; không dừng toàn bộ công việc chỉ vì đề cương không ghi mọi chi tiết.
- Nếu quyết định làm đổi physical model, bỏ kỹ thuật bắt buộc, đổi baseline hoặc mở rộng phạm vi, ghi rõ vướng mắc và chờ người dùng xác nhận quyết định đó; tiếp tục phần độc lập có thể làm.
- Không tự đặt benchmark, hardware limit, API, kết quả đo hoặc citation.
- Khi cần tra cứu kỹ thuật, dùng official specification/documentation/repository hoặc nghiên cứu gốc phù hợp, ghi version/context; không dùng tài liệu ngoài để tự bổ sung yêu cầu sản phẩm.

### 12.3. Mẫu báo cáo mỗi phần việc

```text
Yêu cầu: <ID trong file này; trang/mục đề cương>
Trạng thái trước thay đổi: <đã kiểm tra từ source/config nào>
Thay đổi: <file/function/commit thật; mục đích trực tiếp>
Trạng thái sau thay đổi: <implemented / verified / measured / blocked>
Kiểm tra: <lệnh/cấu hình/reference đã chạy; kết quả thực tế>
Đo lường: <metric, workload, baseline, đường dẫn log/dataset; hoặc chưa đo>
Ảnh hưởng accuracy/physical model: <bằng chứng hoặc điều chưa kiểm chứng>
Phần còn thiếu: <yêu cầu cụ thể; không thêm tính năng ngoài phạm vi>
```

## 13. Tài liệu tham khảo được ghi trong đề cương

Danh mục sau được chép lại từ trang 3 để agent nhận diện nguồn mà đề cương đã dẫn; đây không phải kết quả xác minh độc lập thư mục, nội dung bài báo hoặc trạng thái indexing. Chỉ các yêu cầu ở phần nội dung/kế hoạch mới quyết định phạm vi implementation.

1. N. Aimar, T. Paumard, F. H. Vincent, É. Gourgoulhon, and G. Perrin, “GYOTO 2.0: A polarized relativistic ray-tracing code,” *Classical and Quantum Gravity*, vol. 41, no. 9, Art. no. 095010, 2024.
2. T. Bronzwaer, J. Davelaar, Z. Younsi, M. Mościbrodzka, H. Falcke, C. Kramer, and L. Rezzolla, “RAPTOR-I: Time-dependent radiative transfer in arbitrary spacetimes,” *Astronomy & Astrophysics*, vol. 613, Art. no. A2, 2018.
3. J. R. Dormand and P. J. Prince, “A family of embedded Runge-Kutta formulae,” *Journal of Computational and Applied Mathematics*, vol. 6, no. 1, 1980.
4. E. Hairer, S. P. Nørsett, and G. Wanner, *Solving Ordinary Differential Equations I: Nonstiff Problems*, 2nd ed. Berlin, Germany: Springer, 1993.
5. kavan010, *black_hole*. GitHub, Accessed: Sep. 20, 2026. <https://github.com/kavan010/black_hole>.
6. kavan010, *gravity_sim*. GitHub, Accessed: Sep. 20, 2026. <https://github.com/kavan010/gravity_sim>.
7. J. Kulhanek et al., “LODGE: Level-of-Detail Large-Scale Gaussian Splatting with Efficient Rendering,” in *39th Conference on Neural Information Processing Systems (NeurIPS)*, 2025.
8. A. Madan, N. Sharp, F. Williams, K. Museth, and D. I.W. Levin, “Stochastic Barnes-Hut Approximation for Fast Summation on the GPU,” in *SIGGRAPH Conference Papers '25*, Vancouver, BC, Canada, Aug. 2025.
9. Z. Younsi et al., “OSIRIS: A new code for ray tracing around compact objects,” *The European Physical Journal C*, vol. 82, 2022.

## 14. Bảng truy vết về PDF

| Nội dung PDF | Trang | Mục thực hiện trong file này |
| --- | --- | --- |
| Tên đề tài, cán bộ hướng dẫn, thời gian | 1 | 1 |
| Tổng quan, bottleneck, mục tiêu, kỳ vọng performance | 1-2 | 2, P-01, E-03 |
| Baseline và Visual Studio Performance Profiler | 2 | B-01 đến B-03, P-01 |
| Tối ưu GPU, Grid, Mass Accumulation, Blur, LOD, Adaptive | 2 | P-02, G-01 đến G-07, H-01 đến H-06 |
| Complexity của hai pipeline | 2 | 7 |
| Ablation, numerical error, convergence, metric | 2-3 | G-06/G-07, H-05/H-06, 8 |
| OpenGL sang Vulkan | 2 | P-03 |
| Phạm vi C++, Windows, CPU/GPU, Schwarzschild | 3 | 2.3-2.4 |
| Phần mềm, báo cáo/dataset, 01 bài báo Scopus/SCIE | 3 | 11 |
| Tài liệu tham khảo [1]-[9] | 3 | 13 |
| Giai đoạn 1-2, baseline/profiling, phân công | 4 | 3-4, 10 |
| Giai đoạn 3, state/memory/SSBO/Compute, OpenGL/Vulkan | 4-5 | P-02/P-03, 10 |
| Giai đoạn 4, GravitySim và phân công | 5 | 5, 10 |
| Giai đoạn 5, Black Hole và phân công | 6 | 6, 10 |
| Giai đoạn 6, tích hợp/benchmark/ablation/scalability | 6-7 | 8-10 |
| Giai đoạn 7, báo cáo/demo/slide/reproducibility | 7 | 10-11 |

**Chỉ dẫn ưu tiên cho agent:** hoàn thành các yêu cầu đã truy vết, kiểm tra đúng đắn, đo trước khi kết luận, và dừng việc mở rộng ngoài phạm vi đề cương.
