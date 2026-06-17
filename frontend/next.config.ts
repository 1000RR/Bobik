import type { NextConfig } from "next";

const ContentSecurityPolicy = `
  default-src 'self';
  script-src 'self';
  style-src 'self' 'unsafe-inline';
  img-src 'self' data: blob:;
  font-src 'self' data:;
  connect-src 'self';
  frame-ancestors 'none';
  base-uri 'self';
  form-action 'self';
  object-src 'none';
  upgrade-insecure-requests;
`.replace(/\s{2,}/g, " ").trim();

const SharedConfig: NextConfig = {
  async headers() {
    return [
      {
        source: "/(.*)",
        headers: [
          {
            key: "Content-Security-Policy",
            value: ContentSecurityPolicy,
          },
        ],
      },
    ];
  }
};

const staticConfig: NextConfig = {
  ...SharedConfig,
  output: "export",
  images: {
    unoptimized: true,
  }
};

const ssrConfig: NextConfig = {
  ...SharedConfig
};

export default (process.env.NEXT_BUILD_CONFIG === 'static' ? staticConfig : ssrConfig);
