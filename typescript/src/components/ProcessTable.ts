import { html, LitElement, css, type PropertyValues } from "lit";
import { customElement, state } from "lit/decorators.js";
import { repeat } from "lit/directives/repeat.js"
import { ref } from 'lit/directives/ref.js';
import { AnyProcessInfoUpdate, ProcessInfoEvent, ProcessInfo, PTSelectionChangedEventDetails, PTSelectionChangedEvent } from "../types";

// Process table element that displays the list of processes and their information.
// Singleton element that is exposed as the global PTable for the native side to interact with.
@customElement("process-table")
export class ProcessTable extends LitElement {
    // Called by the native side to send a series of updates to the process list. 
    // This is the main way that the native side communicates changes to the process list to the UI.
    updateTable(...updates: AnyProcessInfoUpdate[]) {
        let dirty = false;
        let added = false;
        for(const update of updates) {
            switch(update.event) { 
                case ProcessInfoEvent.PROCESS_ADDED: {
                    if(this._validateProcessInfo(update.data)) {
                        // @ts-expect-error: name_lc is added in-place for sorting purposes, but isn't actually sent from the native side.
                        update.data.name_lc ??= update.data.name.toLocaleLowerCase();
                        this._processes.set(update.data.pid, update.data);
                        LOG(`Updated pid ${update.data.pid}`);
                        dirty = true;
                        added = true;
                    }
                    break;
                }
                case ProcessInfoEvent.PROCESS_REMOVED: {
                    if(this._validatePID(update.data) && this._processes.delete(update.data)) {
                        LOG(`Removed pid ${update.data}`);
                        dirty = true;
                    }
                    break;
                }
                case ProcessInfoEvent.PROCESS_HIDEALL: {
                    this.hideAllRows();
                    dirty = true;
                    break;
                }
                case ProcessInfoEvent.PROCESS_SHOWALL: {
                    this.showAllRows();
                    dirty = true;
                    break;
                }
                case ProcessInfoEvent.PROCESS_HIDDEN: {
                    this.hideRows(...update.data);
                    dirty = true;
                    break;
                }
                case ProcessInfoEvent.PROCESS_UNHIDDEN: {
                    this.showRows(...update.data);
                    dirty = true;
                    break;
                }
                case ProcessInfoEvent.PROCESS_TITLEUPDATE: {
                    const pi = this._processes.get(update.data.pid);
                    if(!pi) break;
                    pi.title = update.data.title;
                    dirty = true;
                    break;
                }
                case ProcessInfoEvent.PROCESS_DARKMODECHANGE: {
                    document.documentElement.toggleAttribute("dark-mode", update.data);
                    break;
                }
                default: {
                    // @ts-expect-error
                    LOG(`Unhandled event type: ${update.event}`);
                    break;
                }
            }
        }

        // If a process was added, try to apply the current sort to it and only request an update if that didn't result in a 
        // change, since applySort already requests an update if it makes any changes.
        if(added) {
            if(this._applySort()) dirty = false;
        }

        // Request an update if any changes were made that weren't already accounted for by other methods.
        if(dirty) this.requestUpdate('_processes');
    }

    // Helper that asserts that the given processes are shown. Returns whether any changes were made.
    private showRows(...pids: number[]) {
        let shown = 0;
        for(const pid of pids) {
            if(!this._validatePID(pid)) continue;
            const pi = this._processes.get(pid);
            if(pi && pi.hidden) {
                pi.hidden = false;
                ++shown;
            }
        }
        if(!shown) return false;
        LOG(`Shown ${shown} rows!`);
        //this.requestUpdate('_processes');
        return true;
    }

    // Helper that asserts that all processes are shown.
    private showAllRows() {
        for(const pi of this._processes) {
            pi[1].hidden = false;
        }
        LOG('Shown all rows.');
        //this.requestUpdate('_processes');
    }

    // Helper that asserts that the given processes are hidden. Returns whether any changes were made.
    private hideRows(...pids: number[]) {
        let hidden = 0;
        for(const pid of pids) {
            if(!this._validatePID(pid)) continue;
            const pi = this._processes.get(pid);
            if(pi && !pi.hidden) {
                pi.hidden = true;
                ++hidden;
            }
        }
        if(!hidden) return false;
        LOG(`Hid ${hidden} rows!`);
        //this.requestUpdate('_processes');
        return true;
    }

    // Helper that asserts that all processes are hidden.
    private hideAllRows() {
        for(const pi of this._processes) {
            pi[1].hidden = true;
        }
        LOG('Hid all rows.');
        //this.requestUpdate('_processes');
    }

    protected firstUpdated(_changedProperties: PropertyValues): void {
        super.firstUpdated(_changedProperties);
        // @ts-ignore
        globalThis['PTable'] ??= this;
        this.updateComplete.then(() => { 
            this._on_recv_first_batch = () => { 
                NProcessController.signalTableHasElements(); 
                this._on_recv_first_batch = undefined; 
            };
            NProcessController.signalJSReady();
        })
    }

    protected updated(_changedProperties: PropertyValues): void {
        super.updated(_changedProperties);
        this._on_recv_first_batch?.();
    }

    protected override render() {
        return html`
            <div id="table-container">
                <table id="table">
                    <colgroup>
                        <col id="colgroup-pid" />
                        <col id="colgroup-name" />
                    </colgroup>

                    <thead>
                        <tr @click=${this._onHeaderClicked}>
                        ${ProcessTable._headers.map(h => html`
                            <th id="header-${h.key}" key=${h.key} class="header">
                            <div class="header-container">
                                <span>${h.label}</span>
                                ${ProcessTable._arrow_svg}
                            </div>
                            </th>
                        `)}
                        </tr>
                    </thead>

                    <tbody id="body" @click=${this._onRowClicked}>
                        ${repeat(this._processes.entries(), ([pid]) => pid, this._getTemplateForInfo)}
                    </tbody>
                </table>
            </div>
        `;
    }

    // Note - commented out styles is necessary for CEF since it doesn't handle certain post-processing effects well.
    static styles = css`
        table {
            width: 100%;
            max-width: 100%;
            border-collapse: collapse;
            background: var(--neutral-bg);
            /* backdrop-filter: blur(var(--backdrop-blur)); */
            border-radius: var(--corner-radius);
            /* box-shadow: var(--shadow); */
            border: 1px solid var(--neutral-border);
            font-family: "Segoe UI Variable", "Segoe UI", sans-serif;
            font-size: 14px;
            color: var(--text-primary);
            overflow: visible;
            user-select: none;
            table-layout: auto;
        }

        #table-container {
            /* max-height: 500px; */ //will need to adjust this later
            overflow-y: auto;
            width: 100%;
            max-width: 100%;
        }

        #colgroup-pid {
            width: 30ch;
        }

        #colgroup-name {
            width: auto;
            min-width: 0;
        }

        /* enable ellipsis in second column */
        thead th:nth-child(2),
        tbody td:nth-child(2) {
            min-width: 0;
            max-width: 1px;
        }

        thead {
            background: var(--neutral-bg-alt);
            position: sticky; 
            top: -0.17px; /* this sort of works to fix the chromium bug */
            z-index: 5;
        }

        th, td {
            padding: var(--row-padding);
            white-space: nowrap;
            overflow: hidden;
            text-overflow: ellipsis;
        }

        th {
            text-align: left;
            font-weight: 600;
            color: var(--text-secondary);
            border-bottom: 1px solid var(--neutral-border);
            cursor: pointer;
            transition: background-color 0.15s ease/* , box-shadow 0.15s ease */;
        }

        th:hover {
            background: var(--neutral-hover);
        }

        th:active {
            background: var(--neutral-hover);
            /* box-shadow: inset 0 2px 4px rgba(0, 0, 0, 0.1); */
        }

        .header svg {
            visibility: hidden;
            grid-column: 2;
            justify-self: center;
            transition: transform 0.15s ease;
        }

        .header[sort="asc"] svg,
        .header[sort="dsc"] svg {
            visibility: visible;
        }

        .header[sort="dsc"] svg {
            transform: rotate(180deg);
        }

        .header-container {
            display: grid;
            grid-template-columns: 1fr auto 1fr;
            align-items: center;
            width: 100%;
        }

        .header-container span {
            grid-column: 1;
            justify-self: start;
            min-width: 0;
        }

        tbody tr {
            cursor: pointer;
            transition: background-color 0.2s ease, color 0.2s ease;
        }

        tbody tr:hover {
            background: var(--neutral-hover);
        }

        tbody tr.selected {
            background: var(--row-selected-bg);
            outline: 1px solid var(--accent-color);
            outline-offset: -1px;
        }

        tbody tr.selected:hover {
            background: var(--row-selected-bg-hover);
        }

        tbody td {
            border-bottom: 1px solid var(--neutral-border);
        }

        tbody tr:last-child td {
            border-bottom: none;
        }

        tbody tr:focus-within {
            outline: 2px solid var(--accent-color);
            outline-offset: -2px;
        }

        .name-wrapper {
            display: block;
            overflow: hidden;
            text-overflow: ellipsis;
            white-space: nowrap;
            width: 100%;
        }

        .name-wrapper > svg, .name-wrapper > img {
            margin-right: var(--wa-font-size-m);
            width: 16px;
            height: 16px;
        }

        table.striped tbody tr:nth-child(odd) {
            background: rgba(0, 0, 0, 0.02);
        }
    `;


    @state()
    private accessor _processes = new Map<number, ProcessInfo>();

    private _selected_process?: SelectedProcess;
    private _sorted_column?: SortedColumn;
    private _on_recv_first_batch?: () => void;

    private static readonly _arrow_svg = html`
        <svg xmlns="http://www.w3.org/2000/svg" height="24" viewBox="0 0 24 24" width="24" fill="#e3e3e3"> 
            <path d="M0 0h24v24H0z" fill="none"/>
            <path d="M12 8l-6 6 1.41 1.41L12 10.83l4.59 4.58L18 14z"/>
        </svg>`;

    private static readonly _unknown_icon = html`
        <svg xmlns="http://www.w3.org/2000/svg" height="16px" viewBox="0 -960 960 960" width="16px" fill="#e3e3e3">
            <path d="M560-360q17 0 29.5-12.5T602-402q0-17-12.5-29.5T560-444q-17 0-29.5 12.5T518-402q0 17 12.5 29.5T560-360Zm-30-128h60q0-29 6-42.5t28-35.5q30-30 40-48.5t10-43.5q0-45-31.5-73.5T560-760q-41 0-71.5 23T446-676l54 22q9-25 24.5-37.5T560-704q24 0 39 13.5t15 36.5q0 14-8 26.5T578-596q-33 29-40.5 45.5T530-488ZM320-240q-33 0-56.5-23.5T240-320v-480q0-33 23.5-56.5T320-880h480q33 0 56.5 23.5T880-800v480q0 33-23.5 56.5T800-240H320Zm0-80h480v-480H320v480ZM160-80q-33 0-56.5-23.5T80-160v-560h80v560h560v80H160Zm160-720v480-480Z"/>
        </svg>
    `;
    
    private static readonly _headers = [
            { key: 'pid', label: 'PID' },
            { key: 'name_lc', label: 'Process Name' }
        ]

    // Helper that applies the current sort to the process list based on header selection,
    // and applies the appropriate styles.
    private _onHeaderClicked(event: MouseEvent) {
        const header = this._findFirstInstanceInPath(HTMLTableCellElement, event);
        if(!header) {
            LOG_ERROR("Header isn't being picked up, even though the header row was clicked!");
            return;
        }
        if(this._sorted_column) {
            if(header === this._sorted_column.column) {
                this._sorted_column.descending = !this._sorted_column.descending;
                this._sorted_column.column.setAttribute('sort', this._sorted_column.descending ? 'dsc' : 'asc');
                this._applySort();
            }
            else {
                this._sorted_column.column.removeAttribute('sort');
                this._sorted_column.descending = false;
                header.setAttribute('sort', 'asc');
                this._sorted_column.column = header;
                this._applySort();
            }
        }
        else {
            this._sorted_column = {
                column: header,
                descending: false
            };
            header.setAttribute('sort', 'asc');
            this._applySort();
        }
    }

    // Helper that selects the process corresponding to the clicked row.
    private _onRowClicked(event: MouseEvent) {
        this._setSelected(this._findFirstInstanceInPath(HTMLTableRowElement, event));
    }

    // Helper that selects the process corresponding to the given HTMLTableRowElement, or deselects if no row is given.
    private _setSelected(row?: HTMLTableRowElement) {
        if(row === this._selected_process?.row) return;
        this._resetSelected();
        if(!row) {
            return this.dispatchEvent(new CustomEvent<PTSelectionChangedEventDetails>('pt-selection-changed', { detail: { new_selection: undefined }, bubbles: true, composed: true }))
        }

        const info = ((row as any).info as unknown as ProcessInfo);

        if(!info || !this._validateProcessInfo(info)) return;

        this._selected_process = { 
            info: info,
            row: row
        }
        
        row.classList.add('selected');
        this.dispatchEvent(new CustomEvent<PTSelectionChangedEventDetails>('pt-selection-changed', { detail: { new_selection: info }, bubbles: true, composed: true }))
    }

    // Helper that removes the selected style from the currently selected process and removes it from the field that tracks the selected process.
    private _resetSelected() {
        this._selected_process?.row.classList.remove('selected');
        this._selected_process = undefined;
    }

    // Helper that validates that the given ProcessInfo has valid values. 
    // Logs errors for any invalid values and returns whether the info is valid.
    private _validateProcessInfo(info: ProcessInfo) {
        if(!this._validatePID(info.pid)) return false;
        if(info.name.length === 0 || info.name.trim().length === 0) {
            LOG_ERROR(`Can't update process with PID ${info.pid} because it doesn't have a name!`);
            return false;
        }
        return true;
    }

    // Helper that validates that the given PID is valid (greater than or equal to 0).
    private _validatePID(pid: number) {
        if(pid < 0) {
            LOG_ERROR(`Can't update process with PID ${pid} because PIDs must be greater than or equal to 0!`);
            return false;
        }
        return true;
    }

    // Returns a template for a given ProcessInfo for rendering.
    private readonly _getTemplateForInfo = ([pid, info]: [number, ProcessInfo]) => {
        return html`
                    <tr style=${info.hidden ? 'display:none' : ''} title=${info.path} ${ref((element?: any) => { if(element) element.info ??= info; })}>
                        <td>${pid}</td>
                        <td>
                            <div class="name-wrapper">${info.image ? html`<img src="${info.image}"/>` : ProcessTable._unknown_icon}${info.name}${info.title ? ` (${info.title})` : ''}</div></td>
                    </tr>
                `;
    }

    // Helper that finds the first instance of a given constructor in the event's composed path and returns it, 
    // or returns undefined if no instance is found.
    private _findFirstInstanceInPath<T extends EventTargetConstructor, R extends (InstanceType<T> | undefined)>(type: T, event: Event): R {
        const path = event.composedPath();
        let row: R = undefined as R;
        for(const element of path) {
            if(element instanceof type) {
                row = element as R;
                break;
            }
        }
        return row;
    }

    // Helper that sorts the process list backing field based on the currently selected header.
    private _applySort() {
        if(!this._sorted_column) return false;
        const key = this._sorted_column.column.getAttribute('key') as Exclude<keyof ProcessInfo, 'image' | 'hidden' | 'path' | 'title'>;
        this._processes = new Map([...this._processes.entries()].sort(([_lk, lv], [_rk, rv]) => { // find map impl that iterates by sort order?
            if(this._sorted_column!.descending) {
                if(rv[key] === lv[key]) return rv.pid > lv.pid ? 1 : -1;
                return rv[key] > lv[key] ? 1 : -1;
            }
            else {
                if(lv[key] === rv[key]) return lv.pid > rv.pid ? 1 : -1;
                return lv[key] > rv[key] ? 1 : -1;
            }
        }));
        return true;
    }
}

interface SelectedProcess {
    readonly info: ProcessInfo;
    readonly row: HTMLTableRowElement;
}

interface SortedColumn {
    descending: boolean;
    column: HTMLTableCellElement;
}

type EventTargetConstructor<T = EventTarget> = new (...args: any[]) => T;

declare global {
    interface HTMLElementTagNameMap {
        "process-table": ProcessTable;
    }
    interface GlobalEventHandlersEventMap {
        "pt-selection-changed": PTSelectionChangedEvent;
    }
}