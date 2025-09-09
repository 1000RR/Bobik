import type { Metadata } from "next";
import localFont from "next/font/local";
import "./globals.css";

const futura = localFont({
  src: "./fonts/Futura-Medium.woff",
  variable: "--font-futura",
  weight: "100 900",
});

export const metadata: Metadata = {
  title: "Alarm Control Panel",
  description: "Control Panel for the Bobik alarm system",
};

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode;
}>) {
  return (
    <html lang="en">
      <head>
      <link rel="manifest" href="/manifest.json" />
      <link rel="apple-touch-icon" href="/icon180.png" type="image/png"/>
      <link rel="icon" href="/icon192.png" type="image/png"/>
		<meta
  name="viewport"
  content="width=device-width,
           initial-scale=1,
           minimum-scale=1,
           maximum-scale=1,
           user-scalable=no,
           viewport-fit=cover,
           shrink-to-fit=no"/>
      </head>
      <body
        className={`${futura.variable} antialiased`}
      >
        {children}
      </body>
    </html>
  );
}
