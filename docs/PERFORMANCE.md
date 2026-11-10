# Full regulation performance capture

Use the Development Mac package built from the commit being measured. Run it from a terminal with the game in the foreground and no player input. Substitute the path to the built app for `/path/to/Crossover.app`.

```sh
"/path/to/Crossover.app/Contents/MacOS/Crossover" -CrossoverMatchPerf -PerfFullRegulation \
  -Windowed -ResX=1920 -ResY=1080 -ForceRes -Unattended
```

The game uses Low scalability settings with 100% render scale and dynamic resolution off. It warms up for at least 120 rendered frames, then resets the six autonomous athletes with a fixed random seed and a benchmark-only score cap. The benchmark ends when the regulation clock reaches zero. Check-ins pause the regulation clock, so elapsed wall time can exceed four minutes.

Retain these files from the game’s Saved directory. The runtime reports the JSON path in its log.

- `Profiling/perf-full-regulation.json` records wall time, frame count, frame-time percentiles and regulation time.
- The new `CrossoverNightShift-CrossoverNightShift-FPS-*.csv` under `Profiling/FPSChartStats` has per-frame wall, game thread, render thread and GPU timings. Its per-frame section starts at the header `Time (sec),Frame (ms), GT (ms), RT (ms), GPU (ms),DynRes,Context`.

Record the build commit, graphics device, renderer, display checks in the launch log, and the CSV with the report. Verify that the report covers 240,000 milliseconds of regulation, the CSV contains usable CPU and GPU timing samples, and the median and 95th percentile frame times meet the [slice budget](SLICE.md). Missing files, missing timings, or zero GPU timings leave the corresponding measurement open. The short `-CrossoverMatchPerf` diagnostic remains available. The benchmark provides rendered performance evidence and uses autonomous movement. The owner playtest and real-input match and rematch have separate gates.
