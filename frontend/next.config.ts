import type { NextConfig } from "next";

const staticConfig: NextConfig = {
  output: "export",
  images: {
    unoptimized: true,
  }
};

const ssrConfig: NextConfig = {
  async generateBuildId() {
    // Use a version from env or fallback to timestamp
    return `build-${process.env.NEXT_PUBLIC_ASSET_VERSION || Date.now()}`;
  }
};




export default (process.env.NEXT_BUILD_CONFIG === 'static' ? staticConfig : ssrConfig);
