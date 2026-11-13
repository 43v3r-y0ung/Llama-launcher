#include "logadvisor.h"

void LogAdvisor::reset()
{
    m_seen.clear();
    m_hints.clear();
}

void LogAdvisor::add(const QString &id, const QString &text)
{
    if (m_seen.contains(id))
        return;
    m_seen.insert(id);
    m_hints << text;
}

bool LogAdvisor::feedLine(const QString &line)
{
    // Timing lines arrive several times a second during generation and never carry advice.
    if (line.contains(QLatin1String("print_timing")))
        return false;

    const qsizetype before = m_hints.size();

    // common_fit_params: failed to fit params to free device memory: n_gpu_layers already set by user to 99, abort
    if (line.contains(QLatin1String("failed to fit params"))
        && line.contains(QLatin1String("n_gpu_layers already set"))) {
        add(QStringLiteral("fit-ngl"),
            tr("Automatic fitting was skipped because the number of GPU layers is set manually, "
               "so the context size was not reduced to fit into memory. "
               "Set GPU layers to Auto, or choose the context size yourself."));
    }

    // resolve_fused_ops: Flash Attention over K/V rows not supported, set to disabled
    if (line.contains(QLatin1String("Flash Attention"))
        && line.contains(QLatin1String("set to disabled"))) {
        add(QStringLiteral("fa-off"),
            tr("Flash Attention is not supported on this device for this configuration and was turned off, "
               "so the Flash Attention setting has no effect."));
    }

    // common_init_: KV cache shifting is not supported for this context, disabling KV cache shifting
    if (line.contains(QLatin1String("KV cache shifting is not supported"))) {
        add(QStringLiteral("ctx-shift"),
            tr("This model does not support Context Shift, so that option has no effect and can be turned off."));
    }

    // llama_model_loader: tensor overrides to CPU are used with mmap enabled - consider using --load-mode none
    if (line.contains(QLatin1String("tensor overrides to CPU are used with mmap enabled"))) {
        add(QStringLiteral("mmap"),
            tr("Part of the model (for example MoE experts) is kept on the CPU while the file is memory-mapped. "
               "Loading the model fully (Advanced tab, --no-mmap) may be faster."));
    }

    // ggml_vulkan: Device memory allocation of size ... failed / cudaMalloc failed: out of memory / ...
    if (line.contains(QLatin1String("out of memory"), Qt::CaseInsensitive)
        || (line.contains(QLatin1String("allocat"), Qt::CaseInsensitive)
            && line.contains(QLatin1String("failed"), Qt::CaseInsensitive))) {
        add(QStringLiteral("oom"),
            tr("Not enough memory for the requested settings. "
               "Lower the context size or the number of GPU layers, use a quantized KV cache, "
               "or keep MoE experts on the CPU."));
    }

    return m_hints.size() > before;
}
