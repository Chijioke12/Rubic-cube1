import React from 'react';

export default function App() {
  return (
    <div style={{ width: '100vw', height: '100vh', overflow: 'hidden', margin: 0, padding: 0 }}>
      <iframe
        src="/RubiksCube.html"
        style={{ width: '100%', height: '100%', border: 'none' }}
        title="Rubik's Cube C++ Engine"
      />
    </div>
  );
}
