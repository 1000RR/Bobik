// app/components/BuildId.tsx
'use client';
import { useEffect, useState } from 'react';

export default function BuildId() {
  const [id, setId] = useState<string | null>(null);
  const unknownStr = "<<< unknown build id >>>";

  useEffect(() => {
    fetch('/_next/BUILD_ID', { cache: 'no-store' })
      .then(r => r.text())
      .then(txt => {
        if (txt.length > 100) {
          txt = unknownStr;
        }
        setId(txt.trim())
      })
      .catch(() => setId(unknownStr));
  }, []);

  return <span className='centered'>{id ?? 'loading…'}</span>;
}