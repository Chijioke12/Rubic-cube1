import { StrictMode } from 'react';
import { createRoot } from 'react-dom/client';
import { Cpu, Github, ExternalLink } from 'lucide-react';

function PreviewApp() {
  return (
    <div className="min-h-screen bg-neutral-900 text-white flex flex-col items-center justify-center p-6 text-center">
      <div className="bg-neutral-800 border border-neutral-700 rounded-2xl p-8 max-w-lg shadow-2xl">
        <div className="flex justify-center mb-6">
          <div className="p-4 bg-blue-500/10 rounded-full">
            <Cpu className="w-12 h-12 text-blue-400" />
          </div>
        </div>
        <h1 className="text-3xl font-bold mb-4">C++ Rubik's Cube</h1>
        <p className="text-neutral-400 mb-8 leading-relaxed">
          This project is a standalone <strong>C++ Application</strong>. 
          AI Studio's preview window is active to maintain the environment, 
          but the actual game is built via <strong>Emscripten</strong> in your GitHub Actions workflow.
        </p>
        
        <div className="space-y-4">
          <div className="flex items-center gap-3 bg-neutral-900/50 p-4 rounded-xl border border-neutral-700">
            <Github className="w-5 h-5 text-neutral-300" />
            <span className="text-sm font-medium">Builds on GitHub Actions</span>
          </div>
          <div className="flex items-center gap-3 bg-neutral-900/50 p-4 rounded-xl border border-neutral-700">
            <ExternalLink className="w-5 h-5 text-neutral-300" />
            <span className="text-sm font-medium">Live on GitHub Pages (Post-Build)</span>
          </div>
        </div>
        
        <div className="mt-8 pt-6 border-t border-neutral-700">
          <p className="text-xs text-neutral-500 uppercase tracking-widest font-semibold">
            Status: Project Ready for Push
          </p>
        </div>
      </div>
    </div>
  );
}

const rootElement = document.getElementById('root');
if (rootElement) {
  createRoot(rootElement).render(
    <StrictMode>
      <PreviewApp />
    </StrictMode>
  );
}
