import React, { useState } from 'react';
import { Github, RotateCcw, Shuffle } from 'lucide-react';

export default function App() {
  const [clockwise, setClockwise] = useState(true);

  const rotateFace = (faceIndex: number) => {
    const iframe = document.querySelector('iframe');
    if (iframe?.contentWindow) {
      iframe.contentWindow.postMessage({ type: 'ROTATE_FACE', face: faceIndex, clockwise: clockwise ? 1 : 0 }, '*');
    }
  };

  const scramble = () => {
    const iframe = document.querySelector('iframe');
    if (iframe?.contentWindow) {
      iframe.contentWindow.postMessage({ type: 'SCRAMBLE' }, '*');
    }
  };

  const faces = [
    { label: 'Top', index: 0, color: 'bg-white' },
    { label: 'Bottom', index: 1, color: 'bg-yellow-400' },
    { label: 'Front', index: 4, color: 'bg-green-500' },
    { label: 'Back', index: 5, color: 'bg-blue-500' },
    { label: 'Left', index: 2, color: 'bg-orange-500' },
    { label: 'Right', index: 3, color: 'bg-red-600' },
  ];

  return (
    <div className="min-h-screen bg-[#0a0a0a] text-neutral-100 flex flex-col items-center justify-center p-6 font-sans">
      <div className="w-full max-w-md text-center">
        <div className="mb-6">
          <h1 className="text-3xl font-black tracking-tighter mb-2 bg-gradient-to-br from-white to-neutral-500 bg-clip-text text-transparent">
            RUBIK'S 3D
          </h1>
          <p className="text-neutral-500 text-sm font-medium uppercase tracking-[0.2em]">Puzzle Engine</p>
        </div>

        <div className="bg-neutral-900 border border-neutral-800 rounded-3xl p-6 mb-6 shadow-2xl">
          <div className="flex justify-between items-center mb-5">
            <h3 className="text-[10px] font-bold text-neutral-500 uppercase tracking-widest">Face Controls</h3>
            <button 
              onClick={() => setClockwise(!clockwise)}
              className={`flex items-center gap-2 px-4 py-2 rounded-full text-[10px] font-bold transition-all border ${
                clockwise ? 'bg-blue-500/10 text-blue-400 border-blue-500/20' : 'bg-orange-500/10 text-orange-400 border-orange-500/20'
              }`}
            >
              <RotateCcw className={`w-3 h-3 transition-transform duration-300 ${!clockwise ? 'rotate-180' : ''}`} />
              {clockwise ? 'CLOCKWISE' : 'REVERSE'}
            </button>
          </div>
          
          <div className="grid grid-cols-3 gap-4 mb-6">
            {faces.map((face) => (
              <button
                key={face.index}
                onClick={() => rotateFace(face.index)}
                className="group relative bg-neutral-800 hover:bg-neutral-750 border border-neutral-700 rounded-2xl p-4 transition-all active:scale-95 overflow-hidden"
              >
                <div className={`absolute top-0 left-0 w-1 h-full ${face.color} opacity-40`} />
                <span className="block text-xs font-black text-neutral-200 mb-1">{face.label}</span>
                <span className="text-[9px] text-neutral-500 font-mono">Face {face.index}</span>
              </button>
            ))}
          </div>

          <button 
            onClick={scramble}
            className="w-full flex items-center justify-center gap-2 bg-white hover:bg-neutral-200 text-black font-black py-4 rounded-2xl transition-all active:scale-[0.98] shadow-lg shadow-white/5"
          >
            <Shuffle className="w-4 h-4" />
            SCRAMBLE CUBE
          </button>
        </div>
        
        <div className="space-y-4">
          <div className="bg-neutral-900/50 p-5 rounded-2xl border border-neutral-800 text-left">
            <h3 className="text-[10px] font-bold text-neutral-500 uppercase mb-4 tracking-widest">Navigation Guide</h3>
            <div className="space-y-3">
              <div className="flex items-center gap-3">
                <div className="w-8 h-8 rounded-lg bg-neutral-800 flex items-center justify-center text-xs font-bold text-neutral-400 border border-neutral-700">D</div>
                <span className="text-xs text-neutral-400">Drag or swipe to rotate the camera view</span>
              </div>
              <div className="flex items-center gap-3">
                <div className="w-8 h-8 rounded-lg bg-neutral-800 flex items-center justify-center text-xs font-bold text-neutral-400 border border-neutral-700">T</div>
                <span className="text-xs text-neutral-400">Tap face buttons above to rotate cube sides</span>
              </div>
            </div>
          </div>
          
          <div className="flex items-center justify-between px-2">
            <div className="flex items-center gap-2 text-neutral-500">
              <Github className="w-4 h-4" />
              <span className="text-[10px] font-bold uppercase tracking-widest">CI/CD Active</span>
            </div>
            <div className="flex items-center gap-2 text-neutral-500 text-[10px] font-bold uppercase tracking-widest">
              OpenGL ES 2.0
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}
