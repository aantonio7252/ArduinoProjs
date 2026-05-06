import cv2
import numpy as np
import sys

def test_edges(img_path, out_path):
    img = cv2.imread(img_path)
    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    resized = cv2.resize(gray, (128, 64), interpolation=cv2.INTER_AREA)

    # Approach 1: Original Canny
    edges1 = cv2.Canny(resized, 100, 200)

    # Approach 2: Bilateral Filter + Canny
    # This smooths textures but keeps sharp edges
    smoothed = cv2.bilateralFilter(resized, 5, 50, 50)
    edges2 = cv2.Canny(smoothed, 50, 150)
    
    # Approach 3: Morphological Closing to connect gaps
    kernel = cv2.getStructuringElement(cv2.MORPH_CROSS, (3,3))
    edges3 = cv2.morphologyEx(edges2, cv2.MORPH_CLOSE, kernel)

    # Let's combine them into an image
    comb = np.hstack((edges1, edges2, edges3))
    cv2.imwrite(out_path, comb)
    print("Done")

test_edges(sys.argv[1], sys.argv[2])
