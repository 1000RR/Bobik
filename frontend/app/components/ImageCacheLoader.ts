import React, { useEffect } from "react";

type PixelLoaderProps = {
  urls: string[];                                  // e.g. ["/img/a.jpg", "/img/b.png"]
  onReady: (pixels: Record<string, ImageData>) => void; // called once with whatever succeeded
};

/** Invisible helper: loads images -> decodes -> returns ImageData per URL. */
const ImageCacheLoader: React.FC<PixelLoaderProps> = ({ urls, onReady }) => {
  useEffect(() => {
    (async () => {
      const out: Record<string, ImageData> = {};

      const tasks = urls.map(async (url) => {
        try {
          const resp = await fetch(url, { cache: "force-cache" });
          const blob = await resp.blob();

          // Decode
          let bmp: ImageBitmap | HTMLImageElement;
          if ("createImageBitmap" in window) {
            bmp = await createImageBitmap(blob);
          } else {
            bmp = await new Promise<HTMLImageElement>((resolve, reject) => {
              const img = new Image();
              img.onload = () => resolve(img);
              img.onerror = reject;
              img.src = URL.createObjectURL(blob);
            });
          }

          // Draw to canvas and capture pixels
          const w = (bmp as any).width, h = (bmp as any).height;
          const canvas = document.createElement("canvas");
          canvas.width = w; canvas.height = h;
          const ctx = canvas.getContext("2d")!;
          ctx.drawImage(bmp as any, 0, 0);
          out[url] = ctx.getImageData(0, 0, w, h);
        } catch {
          // Ignore failures as requested
        }
      });

      await Promise.allSettled(tasks);
      onReady(out); // fire once with whatever succeeded
    })();
  }, [urls, onReady]);

  return null; // nothing rendered
};

export default ImageCacheLoader;