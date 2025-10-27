import './index.css';
import '@awesome.me/webawesome/dist/components/input/input.js';

import './helpers/WADarkModeListener'

import './components/ProcessTable'
import './components/PTControl'
import { type ProcessTable } from './components/ProcessTable';
import { ProcessInfoEvent } from "./types";

declare global {
    const BUILD_MODE: 'production' | 'development';
    const ENABLE_LOGGING: boolean;
    const NProcessController: {
        signalJSReady(): void;
        signalTableHasElements(): void;
        endTask(pid: number, exe_name: string): void;
        updateSearchEngineText(text: string): void;
    }
    const NProcessManipulator: {
        chooseDLLFile(): void;
        doInject(pid: number, exe_name: string, attach_stdout: boolean): void;
    }
    const PTable: ProcessTable;

    const LOG: typeof console.log;
    const LOG_INFO: typeof console.log;
    const LOG_DEBUG: typeof console.debug;
    const LOG_ERROR: typeof console.error;
    const LOG_WARN: typeof console.warn;
}

if(BUILD_MODE === 'development') {
    // @ts-ignore
    window['NProcessController'] ??= {
        signalJSReady(): void {
            LOG_DEBUG('Sent JS ready.');
            const testprocs = [];
            for(let i = 0; i < 999; ++i) {
                testprocs.push({event: ProcessInfoEvent.PROCESS_ADDED, data: {pid: Math.floor(Math.random() * Number.MAX_SAFE_INTEGER), name: crypto?.randomUUID() as string ?? i.toString(), path: "C:testpath"}})
            }
            //table!.modifyTable({pid: 234, name: 'proc 2'}, {pid: 345, name: 'anem'}, {pid: 1, name: 'kernel'}, ...testprocs);
            PTable.updateTable(...testprocs as any);
        },
        signalTableHasElements(): void {
            LOG_DEBUG('Sent table first updated.');
        },
        endTask(pid: number, exe_name: string): void {
            LOG_DEBUG(`Ending task ${pid} ${exe_name}!`);
        },
        updateSearchEngineText(text: string): void {
            LOG_DEBUG(`Updating search with text: ${text}`);
        }
    };

    // @ts-ignore
    window['NProcessManipulator'] ??= {
        chooseDLLFile(): void {
            console.debug('Started DLL file picker!');
        },
        doInject(pid: number, exe_name: string, attach_stdout: boolean): void {
            console.debug(`Requesting injection for ${pid} ${exe_name} with attach = ${attach_stdout}!`);
        },
    }
}

if(ENABLE_LOGGING) {
    // @ts-ignore
    window["LOG"] ??= console.log.bind(console);

    // @ts-ignore
    window["LOG_INFO"] ??= console.log.bind(console);

    // @ts-ignore
    window["LOG_DEBUG"] ??= console.debug.bind(console);

    // @ts-ignore
    window["LOG_ERROR"] ??= console.error.bind(console);

    // @ts-ignore
    window["LOG_WARN"] ??= console.warn.bind(console);
}

else {
    // @ts-ignore
    window["LOG"] ??= function(...args: any[]){};

    // @ts-ignore
    window["LOG_INFO"] ??= function(...args: any[]){};

    // @ts-ignore
    window["LOG_DEBUG"] ??= function(...args: any[]){};

    // @ts-ignore
    window["LOG_ERROR"] ??= function(...args: any[]){};

    // @ts-ignore
    window["LOG_WARN"] ??= function(...args: any[]){};
}

document.addEventListener('DOMContentLoaded', () => {
    const input = document.querySelector('wa-input');
    if(!input) throw new Error("Can't find search element!");
    let timeout: ReturnType<typeof setTimeout> | undefined;
    input.addEventListener('input', () => {
        clearTimeout(timeout);
        timeout = setTimeout(() => NProcessController.updateSearchEngineText(input.value ?? ""), 100)
    });
}, {once: true})