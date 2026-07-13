# UB Plan Visualizer Demo

Open `ub_plan_visualizer.html` directly in a browser. It is a static page: no
server, build step, or external package is required.

The page accepts either:

- JSON from `plan_precvpipeline_ub.py --format=json`
- Text output from `plan_precvpipeline_ub.py --format=text`
- `sample_ub_plan.json`

Example model output generation:

```bash
python3 cvpipeline_ub_model_cpp/scripts/generate_ub_demo_json.py \
  --pre-cvpipeline-ir=Output/index/d0_before_cvpipeline_generic/objects/<sha>/generic.mlir \
  --output=cvpipeline_ub_model_cpp/output/demo/ub_plan.json
```

Then open the HTML page and load the generated JSON with the `Open JSON`
button.
