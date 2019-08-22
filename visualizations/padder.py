
import os

for fi in os.listdir("."):
    if ".png" in fi:
        ct = fi.split("_")[1].replace(".png","")
        os.rename(fi, fi.split("_")[0]+"_"+"0"*(3-len(ct))+ct+".png")

