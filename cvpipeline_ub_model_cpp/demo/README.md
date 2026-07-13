# UB Plan Visualizer Demo

`ub_plan_visualizer.html` is the reusable static template. Open it directly when
you want to use the `Open JSON` button manually.

The page accepts either:

- JSON from `plan_precvpipeline_ub.py --format=json`
- Text output from `plan_precvpipeline_ub.py --format=text`
- `sample_ub_plan.json`

Example model output generation:

```bash
python3 cvpipeline_ub_model_cpp/scripts/generate_ub_demo_json.py \
  --pre-cvpipeline-ir=cvpipeline_ub_model_cpp/examples/inputs/demo_randn_before_cvpipeline.mlir \
  --output=cvpipeline_ub_model_cpp/output/demo/ub_plan.json
```

Then open the HTML page and load the generated JSON with the `Open JSON`
button.

For a one-command demo that always embeds the latest JSON into a fresh HTML
snapshot, run:

```bash
bash cvpipeline_ub_model_cpp/run_demo_ub_plan.sh
```

Then open:

```text
cvpipeline_ub_model_cpp/output/demo/ub_plan_visualizer.html
```
