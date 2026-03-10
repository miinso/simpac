# simpac

A meek peon, simulation package of mine.

![preview](https://github.com/user-attachments/assets/ba823951-c985-487c-a889-f8821ca9ad97)

## What's inside

A collection of things I found studying computer animation that make real-time simulation of rigid bodies, soft bodies, and cloth possible.
I try various integration schemes, solver regimes, and real-time orchestration of them.

## Structure

```
small/       individual experiments
medium/      some mid-scale projects
large/       larger projects
modules/     reusable libraries
papers/      paper implementations
third_party/ external deps
resources/   shared assets
```

## Build

Requires [bazel](https://bazel.build/).

```bash
# cloth sim
bazel run //small/cloth:main -c opt
```

## Demos

Live wasm demos at [minseopark.vercel.app/lab](https://minseopark.vercel.app/lab)
