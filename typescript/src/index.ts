// @ts-ignore
import './index.css';
import '@awesome.me/webawesome/dist/components/input/input.js';

import './components/ProcessTable'
import './components/PTControl'
import { type ProcessTable } from './components/ProcessTable';
import { ProcessInfoEvent } from "./types";

declare global {
    const BUILD_MODE: 'production' | 'development';
    const ENABLE_LOGGING: boolean;

    // Controller that is defined in native CEF environment and is callable from JS to send messages to the native side.
    const NProcessController: {
        // Signals to the native side that the JS environment is ready. Should be called as soon as the ProcessTable is first updated.
        // The native side will then send the initial batch of processes to populate the table.
        signalJSReady(): void;

        // Signals to the native side that the table has received its first batch of processes and has updated.
        // This tells the native side to show the window and allow user filtering to apply.
        signalTableHasElements(): void;

        // Tells the native side to end a task with the given PID and executable name. 
        // Executable name is just for logging purposes on the native side and isn't used for anything else.
        endTask(pid: number, exe_name: string): void;

        // Tells the native side to update filters based on the given search text. 
        // The native side will then update the process list in a multi-threaded manner and send back updates as necessary.
        // Note that this means that filtering intentionally doesn't work in development mode.
        updateSearchEngineText(text: string): void;
    }

    const PTable: ProcessTable;

    const LOG: typeof console.log;
    const LOG_INFO: typeof console.log;
    const LOG_DEBUG: typeof console.debug;
    const LOG_ERROR: typeof console.error;
    const LOG_WARN: typeof console.warn;
}

// Use mock implementations in development mode
if(BUILD_MODE === 'development') {
    // @ts-ignore
    window['NProcessController'] ??= {
        // Mock version of signalJSReady that generates a bunch of random processes for testing.
        signalJSReady(): void {
            LOG_DEBUG('Sent JS ready.');
            const testprocs = [];
            for(let i = 0; i < 999; ++i) {
                testprocs.push({event: ProcessInfoEvent.PROCESS_ADDED, data: {pid: Math.floor(Math.random() * Number.MAX_SAFE_INTEGER), name: crypto?.randomUUID() as string ?? i.toString(), path: "C:testpath"}})
            }
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
}

// Use no-op implementations for logging if logging is disabled.
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

    // Debounce search input to avoid sending too many messages to the native side while the user is typing.
    input.addEventListener('input', () => {
        clearTimeout(timeout);
        timeout = setTimeout(() => NProcessController.updateSearchEngineText(input.value ?? ""), 100)
    });

    // In development mode, manually create the PROCESS_DARKMODECHANGE event based on the current color scheme and changes to it.
    // In the native environment, CEF doesn't support the prefers-color-scheme media query, so the native side has to implement this
    // and signal accordingly.
    // Doing this approach rather than prefers-color-scheme in development keeps the CSS the same between 
    // development and production and allows testing of dark mode changes without having to switch the entire OS color scheme.
    if(BUILD_MODE === 'development') {
        customElements.whenDefined('process-table').then(() => {
            const dark_mode = matchMedia("(prefers-color-scheme: dark)");
            document.querySelector<ProcessTable>('process-table')?.updateComplete.then(() => {
                PTable.updateTable({event: ProcessInfoEvent.PROCESS_DARKMODECHANGE, data: dark_mode.matches});
            });
            dark_mode.addEventListener('change', e => {
                PTable.updateTable({event: ProcessInfoEvent.PROCESS_DARKMODECHANGE, data: e.matches});
            });
        })
    }
}, {once: true})