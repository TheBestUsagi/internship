# Week 7/8 — RT Linux 探索（上半部分：调研与 RT 补丁概览）

> 交付物：本页 Markdown 文档＋图示，提交到你的 GitHub 仓库 `docs/week7-rt-linux/README.md`。

---

## 1. 实时系统概念与分类

**实时（Real-Time）**：不仅追求“算对”，更追求“按时算对”。

- **关键指标**
  - **WCET**（Worst-Case Execution Time）：任务在最坏情况下的执行时间上界
  - **截止期（Deadline）**：任务必须在此之前完成
  - **抖动（Jitter）**：任务实际完成时间相对期望时间的波动
  - **中断/调度延迟（Latency）**：从事件发生到任务开始执行的时间

- **硬实时（Hard RT）**：**错过一次截止期即失败**（如飞控、医疗植入设备）  
- **软实时（Soft RT）**：偶尔错过截止期可接受，但性能下降（如多媒体播放）

> 直观对比：硬实时看“最坏情况保证”；软实时看“平均与波动”。

---

## 2. 什么是 RT Linux

**RT Linux** 泛指以 Linux 为基础、通过内核配置或补丁增强实时性的系统。主要两条路线：

1) **PREEMPT_RT（主线补丁）**：给标准 Linux 打上 `-rt` 补丁，尽量保持单内核架构与生态，目标是**把几乎所有内核上下文都做成可抢占**，显著降低抢占/中断延迟。  
2) **双内核/共核框架**（如 Xenomai/RTAI）：在 Linux 旁边再跑一个实时内核（或抽象层），**把外设中断首先发给实时域**，Linux 被降为低优先级后台任务，换来更严格的最坏时延上界。

**优点（PREEMPT_RT）**
- 生态一致（glibc、POSIX、驱动、容器等仍可用）
- 迁移成本低，学习曲线平缓

**挑战**
- 最坏时延通常**高于**专用 RTOS/双内核方案
- 个别驱动或子系统仍可能有不可抢占的“硬区段”

---

## 3. PREEMPT_RT 采用的关键技术点

- **完全可抢占内核**（`CONFIG_PREEMPT_RT`）：把原本的关抢占区尽可能缩短/消除  
- **中断线程化（Threaded IRQ）**：大多数硬中断服务被转为内核线程（`irq/*`），可设优先级，避免长时间关中断  
- **自旋锁 → 实时互斥（`rt_mutex`）+ 优先级继承**：缓解优先级反转；仅少数极短临界区保留 `raw_spinlock`  
- **高精度定时器（hrtimer）**：纳秒级计时，配合 `clock_nanosleep()`、`SCHED_DEADLINE` 等实现精确定时  
- **软中断/下半部线程化**：大量 softirq 在内核线程上下文执行，避免不可控长延迟  
- **RCU Boost**：读端阻塞可能超时时提升优先级以缩短等待  
- **调度策略**  
  - `SCHED_FIFO` / `SCHED_RR`：固定优先级实时调度  
  - `SCHED_DEADLINE`：近似 EDF（Earliest Deadline First），通过 `sched_setattr` 配置运行/周期/截止期  
- **CPU 亲和/隔离**：`isolcpus=`, `nohz_full=`, `rcu_nocbs=` 把系统噪声隔离到非 RT 核心  
- **内存与 I/O 实时性**：`mlockall(MCL_CURRENT|MCL_FUTURE)` 锁页、预热页表；使用 `O_NONBLOCK`、`O_DIRECT` 减少阻塞路径  
- **可观测性**：`ftrace`, `trace-cmd`, `perf`, `latencytop`，配合 cyclictest 量化时延

---

## 4. 市面 RT OS / 框架横向扫描

| 方案 | 架构 | 许可证 | 特点 | 典型场景 |
|---|---|---|---|---|
| **Linux + PREEMPT_RT** | 单内核（增强抢占） | GPL | 生态完整，实时性显著增强，最坏时延取决于硬件/驱动 | 工业控制、机器人、音视频、移动平台边缘侧 |
| **Xenomai (Cobalt)** | 双内核/共核 | GPL/LGPL | 中断先给实时域，Linux 退居后台，时延更稳 | 运动控制、机床、测试测量 |
| **QNX Neutrino** | 微内核 | 商业 | POSIX 强、时延可预测、认证多 | 车载、航空、医疗 |
| **VxWorks** | 单内核（商业 RTOS） | 商业 | 成熟、认证齐全、工具链完善 | 航天、工业控制 |
| **RTEMS** | 单内核 RTOS | 开源 | 面向嵌入式、航空航天常用 | 卫星、深空探测 |
| **Zephyr** | RTOS | Apache 2.0 | IoT 面向、驱动多、内核小 | 传感器/可穿戴 |
| **FreeRTOS** | RTOS | MIT | 轻量、社区大、MCU 场景主流 | MCU 级实时控制 |

> 选择建议：若需要 Linux 生态与较好实时性，先考虑 **PREEMPT_RT**；若强最坏时延与严格确定性，考虑 **Xenomai/QNX/VxWorks** 等。

---

## 5. 下载 Linux 6.12 对应的 RT 补丁（示例流程）

> 说明：`-rt` 补丁通常以 `linux-stable-rt` 仓库或 `patch-<ver>-rt<rev>.patch` 发布。**若 6.12 的 -rt 尚未发布**，可先使用临近的 LTS 版本（如 6.6/6.8）验证流程，方法相同。

### 5.1 获取源码与补丁

```bash
# ① 获取标准内核（示例切到 v6.12 标签）
git clone https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git
cd linux && git checkout v6.12

# ② 方式A：直接克隆 -rt 分支（若已发布）
# 建议单独目录：
cd ..
git clone https://git.kernel.org/pub/scm/linux/kernel/git/rt/linux-stable-rt.git rt-linux
cd rt-linux && git checkout v6.12-rtX   # X 为具体修订号

# ② 方式B：下载独立补丁（若提供 tar.xz/patch.xz）
# 把 patch-6.12-rtX.patch(.xz) 放到上一级目录

