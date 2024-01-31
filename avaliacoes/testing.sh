#!/usr/bin/env bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

pushd "$1"

make 

popd

echo ""
echo "Info Test"
"$1/imageTool" "$SCRIPT_DIR/testepares/UA.pgm" info 2>/dev/null
echo ""

echo ""
echo "Negative Test"
"$1/imageTool" "$SCRIPT_DIR/testepares/UA.pgm" neg save UA_neg.pgm 2>/dev/null
python3 "$SCRIPT_DIR/imageDiff.py" UA_neg.pgm "$SCRIPT_DIR/testepares/UA_neg.pgm"
echo ""

echo ""
echo "Threshold Test"
"$1/imageTool" "$SCRIPT_DIR/testepares/UA.pgm" thr 130 save UA_thr.pgm 2>/dev/null
python3 "$SCRIPT_DIR/imageDiff.py" UA_thr.pgm "$SCRIPT_DIR/testepares/UA_thr.pgm"
echo ""

echo ""
echo "Brighten Test"
"$1/imageTool" "$SCRIPT_DIR/testepares/UA.pgm" bri 0.33 save UA_bri.pgm 2>/dev/null
python3 "$SCRIPT_DIR/imageDiff.py" UA_bri.pgm "$SCRIPT_DIR/testepares/UA_bri.pgm"
echo ""

echo ""
echo "Mirror Test"
"$1/imageTool" "$SCRIPT_DIR/testepares/UA.pgm" mirror save UA_mirror.pgm 2>/dev/null
python3 "$SCRIPT_DIR/imageDiff.py" UA_mirror.pgm "$SCRIPT_DIR/testepares/UA_mirror.pgm"
echo ""

echo ""
echo "Rotate Test"
"$1/imageTool" "$SCRIPT_DIR/testepares/UA.pgm" rotate save UA_rotate.pgm 2>/dev/null
python3 "$SCRIPT_DIR/imageDiff.py" UA_rotate.pgm "$SCRIPT_DIR/testepares/UA_rotate.pgm"
echo ""

echo ""
echo "ImageValidRect Test (should fail)"
"$1/imageTool" "$SCRIPT_DIR/testepares/UA.pgm" crop 1600,1000,200,200
echo ""

echo ""
echo "ImageCrop Test"
"$1/imageTool" "$SCRIPT_DIR/testepares/UA.pgm" crop 820,530,250,200 save UA_crop.pgm 2>/dev/null
python3 "$SCRIPT_DIR/imageDiff.py" UA_crop.pgm "$SCRIPT_DIR/testepares/UA_crop.pgm"
echo ""

echo ""
echo "ImagePaste Test"
"$1/imageTool" "$SCRIPT_DIR/testepares/AED.pgm" "$SCRIPT_DIR/testepares/UA.pgm" paste 550,200 save UA_paste.pgm 2>/dev/null
python3 "$SCRIPT_DIR/imageDiff.py" UA_paste.pgm "$SCRIPT_DIR/testepares/UA_paste.pgm"
echo ""

echo ""
echo "ImageBlend Test"
"$1/imageTool" "$SCRIPT_DIR/testepares/AED.pgm" "$SCRIPT_DIR/testepares/UA.pgm" blend 550,200,0.5 save UA_blend.pgm 2>/dev/null
python3 "$SCRIPT_DIR/imageDiff.py" UA_blend.pgm "$SCRIPT_DIR/testepares/UA_blend.pgm"
echo ""

echo ""
echo "ImageLocate/ImageMatchSubImage Test"

"$1/imageTool" "$SCRIPT_DIR/testepares/UA.pgm" "$SCRIPT_DIR/testepares/UA.pgm" locate 2>/dev/null
echo "Expected 0,0"

"$1/imageTool" "$SCRIPT_DIR/testepares/UA_crop.pgm" "$SCRIPT_DIR/testepares/UA.pgm" locate 2>/dev/null
echo "Expected 820,530"

echo ""

echo ""
echo "ImageBlur Test"

"$1/imageTool" "$SCRIPT_DIR/testepares/UA.pgm" tic blur 9,9 toc save UA_blur_9_9.pgm 2>/dev/null
python3 "$SCRIPT_DIR/imageDiff.py" UA_blur_9_9.pgm "$SCRIPT_DIR/testepares/UA_blur_9_9.pgm"

"$1/imageTool" "$SCRIPT_DIR/testepares/UA.pgm" tic blur 11,13 toc save UA_blur.pgm 2>/dev/null
python3 "$SCRIPT_DIR/imageDiff.py" UA_blur.pgm "$SCRIPT_DIR/testepares/UA_blur.pgm"

echo ""

echo ""
echo "Valgrind Tests"

valgrind "$1/imageTool" create 100,100
valgrind "$1/imageTool" "$SCRIPT_DIR/testepares/UA.pgm" blur 7,7
valgrind "$1/imageTool" "$SCRIPT_DIR/testepares/UA.pgm" mirror
valgrind "$1/imageTool" "$SCRIPT_DIR/testepares/UA.pgm" rotate
valgrind "$1/imageTool" "$SCRIPT_DIR/testepares/UA.pgm" crop 820,530,250,200

echo ""
