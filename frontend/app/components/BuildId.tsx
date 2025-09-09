// app/components/BuildId.tsx
'use client';
import { useEffect, useState } from 'react';

export default function BuildId() {
  const [id, setId] = useState<string | null>(null);

  useEffect(() => {
    fetch('/_next/BUILD_ID', { cache: 'no-store' })
      .then(r => r.text())
      .then(txt => setId(txt.trim()))
      .catch(() => setId('unknown'));
  }, []);

  return <span className='centered'>{id ?? 'loading…'}</span>;
}