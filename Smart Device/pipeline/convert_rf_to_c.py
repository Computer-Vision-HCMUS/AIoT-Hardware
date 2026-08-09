import sys
import joblib
import numpy as np

if len(sys.argv) < 3:
    print('Usage: convert_rf_to_c.py <model_joblib_path> <out_header_path>')
    sys.exit(2)

model_path = sys.argv[1]
out_path = sys.argv[2]

model = joblib.load(model_path)

n_trees = len(model.estimators_)
try:
    n_classes = int(model.n_classes_)
except:
    n_classes = int(len(model.classes_))

features_list = []
thresholds_list = []
left_list = []
right_list = []
values_list = []
offsets = []
node_counts = []

for est in model.estimators_:
    tree = est.tree_
    offset = len(features_list)
    offsets.append(offset)
    node_count = tree.node_count
    node_counts.append(node_count)
    for i in range(node_count):
        feat = int(tree.feature[i])
        thresh = float(tree.threshold[i])
        left = int(tree.children_left[i])
        right = int(tree.children_right[i])
        val = tree.value[i][0]
        # cast val to uint16 counts (round)
        val_int = [int(round(x)) for x in val]

        features_list.append(feat)
        thresholds_list.append(thresh)
        left_list.append(left)
        right_list.append(right)
        # append per-class
        for c in range(n_classes):
            values_list.append(val_int[c])

# Now write header
with open(out_path, 'w') as f:
    f.write('/* Auto-generated data-driven RandomForest header */\n')
    f.write('#pragma once\n')
    f.write('#include <stdint.h>\n')
    f.write('\n')
    total_nodes = len(features_list)
    f.write('// Model metadata\n')
    f.write(f'static const int32_t rf_n_trees = {n_trees};\n')
    f.write(f'static const int32_t rf_n_classes = {n_classes};\n')
    f.write(f'static const int32_t rf_total_nodes = {total_nodes};\n')
    f.write('\n')

    def dump_array(name, arr, ctype, perline=12):
        f.write(f'static const {ctype} {name}[] = {{\n')
        for i in range(0, len(arr), perline):
            slice = arr[i:i+perline]
            line = ', '.join(str(x) for x in slice)
            f.write('    ' + line + ',\n')
        f.write('};\n\n')

    dump_array('rf_offsets', offsets, 'int32_t')
    dump_array('rf_node_counts', node_counts, 'int32_t')
    dump_array('rf_feature', features_list, 'int16_t')
    # thresholds as float
    f.write('static const float rf_threshold[] = {\n')
    for i in range(0, len(thresholds_list), 8):
        slice = thresholds_list[i:i+8]
        line = ', '.join(f'{x:.6f}f' for x in slice)
        f.write('    ' + line + ',\n')
    f.write('};\n\n')
    dump_array('rf_left', left_list, 'int32_t')
    dump_array('rf_right', right_list, 'int32_t')
    dump_array('rf_values_flat', values_list, 'uint16_t', perline=8)

    # Add simple interpreter functions: rf_predict and rf_predict_proba
    f.write('\n')
    f.write('static inline int32_t rf_predict(const float *features, int32_t features_length) {\n')
    f.write('    uint32_t class_votes[16] = {0}; // support up to 16 classes\n')
    f.write('    for (int t = 0; t < rf_n_trees; ++t) {\n')
    f.write('        int32_t node = rf_offsets[t];\n')
    f.write('        int32_t base = rf_offsets[t];\n')
    f.write('        while (1) {\n')
    f.write('            int16_t feat = rf_feature[node];\n')
    f.write('            if (feat < 0) { // leaf (sklearn uses -2)\n')
    f.write('                break;\n')
    f.write('            }\n')
    f.write('            float thresh = rf_threshold[node];\n')
    f.write('            if (features[feat] <= thresh) {\n')
    f.write('                int32_t child = rf_left[node];\n')
    f.write('                if (child < 0) break;\n')
    f.write('                node = base + child;\n')
    f.write('            } else {\n')
    f.write('                int32_t child = rf_right[node];\n')
    f.write('                if (child < 0) break;\n')
    f.write('                node = base + child;\n')
    f.write('            }\n')
    f.write('        }\n')
    f.write('        // node is leaf index (absolute). find local index\n')
    f.write('        int local = node;\n')
    f.write('        // accumulate votes by leaf distribution\n')
    f.write('        for (int c = 0; c < rf_n_classes; ++c) {\n')
    f.write('            uint16_t v = rf_values_flat[local * rf_n_classes + c];\n')
    f.write('            class_votes[c] += v;\n')
    f.write('        }\n')
    f.write('    }\n')
    f.write('    // pick argmax\n')
    f.write('    int best = 0;\n')
    f.write('    uint32_t bestv = class_votes[0];\n')
    f.write('    for (int c = 1; c < rf_n_classes; ++c) {\n')
    f.write('        if (class_votes[c] > bestv) { bestv = class_votes[c]; best = c; }\n')
    f.write('    }\n')
    f.write('    return best;\n')
    f.write('}\n')

    f.write('\nstatic inline void rf_predict_proba(const float *features, int32_t features_length, float *out_probs, int32_t out_len) {\n')
    f.write('    // sum class counts across trees and normalize\n')
    f.write('    uint32_t class_acc[16] = {0};\n')
    f.write('    for (int t = 0; t < rf_n_trees; ++t) {\n')
    f.write('        int32_t node = rf_offsets[t];\n')
    f.write('        int32_t base = rf_offsets[t];\n')
    f.write('        while (1) {\n')
    f.write('            int16_t feat = rf_feature[node];\n')
    f.write('            if (feat < 0) break;\n')
    f.write('            float thresh = rf_threshold[node];\n')
    f.write('            if (features[feat] <= thresh) {\n')
    f.write('                int32_t child = rf_left[node]; if (child < 0) break; node = base + child;\n')
    f.write('            } else {\n')
    f.write('                int32_t child = rf_right[node]; if (child < 0) break; node = base + child;\n')
    f.write('            }\n')
    f.write('        }\n')
    f.write('        int local = node;\n')
    f.write('        for (int c = 0; c < rf_n_classes; ++c) {\n')
    f.write('            class_acc[c] += rf_values_flat[local * rf_n_classes + c];\n')
    f.write('        }\n')
    f.write('    }\n')
    f.write('    uint32_t total = 0; for (int c = 0; c < rf_n_classes; ++c) total += class_acc[c];\n')
    f.write('    if (total == 0) { for (int c = 0; c < rf_n_classes; ++c) out_probs[c] = 0.0f; return; }\n')
    f.write('    for (int c = 0; c < rf_n_classes; ++c) out_probs[c] = (float)class_acc[c] / (float)total;\n')
    f.write('}\n')

print('Wrote', out_path)
