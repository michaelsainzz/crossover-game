# Full regulation performance capture

Use the Development Mac package built from the commit being measured. Run it from a terminal with the game in the foreground and no player input. Substitute the path to the built app for `/path/to/Crossover.app`.

```sh
"/path/to/Crossover.app/Contents/MacOS/Crossover" -CrossoverMatchPerf -PerfFullRegulation \
  -Windowed -ResX=1920 -ResY=1080 -ForceRes -Unattended
```

The game uses Low scalability settings with 100% render scale and dynamic resolution off. It warms up for at least 120 rendered frames, then resets the six autonomous athletes with a fixed random seed and a benchmark-only score cap. The benchmark ends when the regulation clock reaches zero. Check-ins pause the regulation clock, so elapsed wall time can exceed four minutes.

Retain these files from the game’s Saved directory. The runtime reports the JSON path in its log.

- `Profiling/perf-full-regulation.json` records wall time, frame count, frame-time percentiles and regulation time.
- The new `CrossoverNightShift-FPS-*.csv` under `Profiling/FPSChartStats` has per-frame wall, game thread, render thread and GPU timings. Its per-frame section starts at the header `Time (sec),Frame (ms), GT (ms), RT (ms), GPU (ms),DynRes,Context`.

Record the build commit, graphics device, renderer, display checks in the launch log, and the CSV with the report. Verify that the report covers 240,000 milliseconds of regulation, the CSV contains usable CPU and GPU timing samples, and the median and 95th percentile frame times meet the [slice budget](SLICE.md). Missing files, missing timings, or zero GPU timings leave the corresponding measurement open. The short `-CrossoverMatchPerf` diagnostic remains available. The benchmark provides rendered performance evidence and uses autonomous movement. The owner playtest and real-input match and rematch have separate gates.

## Night Shift reference run

The Mac Development build at `4a0d62f` ran on the reference Apple M4 with UE 5.8.4 and Metal SM6. The renderer verified 1920×1080 windowed, Low scalability, native render scale and dynamic resolution off. Six autonomous players completed 240,000 ms of regulation after the warmup. The run captured 27,491 frames over 247.229 seconds and exited 0.

| Timing | Median | 95th percentile |
| --- | ---: | ---: |
| Frame wall time, benchmark report | 9.00 ms | 9.45 ms |
| Game thread, Unreal timing chart | 1.02 ms | 1.36 ms |
| Render thread, Unreal timing chart | 9.00 ms | 9.40 ms |
| GPU, Unreal timing chart | 8.76 ms | 9.02 ms |

Frame timing met the 16.7 ms median and 22.0 ms 95th percentile targets. The JSON summary, frame CSV and launch output are retained with the private run evidence. Owner playtest and physical controller verification remain separate.
