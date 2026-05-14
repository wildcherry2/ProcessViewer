import { css, html, LitElement, PropertyValues } from "lit";
import { customElement, property, query } from "lit/decorators.js";

import './NativeButton'
import '@awesome.me/webawesome/dist/components/checkbox/checkbox.js'

import type { ProcessInfo, PTSelectionChangedEvent } from "../types";
import type { NativeButton } from "./NativeButton";

// The control bar below the process table that contains buttons for actions that can be taken on the selected process.
// For ProcessViewer, this is just the "End Process" button.
@customElement('pt-control')
export class PTControl extends LitElement {
    // Listen for selection change events from the process table to know which process is currently selected, 
    // and enable/disable buttons accordingly.
    connectedCallback(): void {
        super.connectedCallback();
        document.addEventListener('pt-selection-changed', this._onSelectionChanged);
    }

    // Clean up event listener when the element is removed from the DOM.
    disconnectedCallback(): void {
        super.disconnectedCallback();
        document.removeEventListener('pt-selection-changed', this._onSelectionChanged);
    }

    protected override render() {
        return html`
            <div class="left">
            </div>
            <div class="right">
                <native-button id="end_process_btn" class="primary disabled" @click="${this._onEndTask}">End Process</native-button>
            </div>
        `
    }

    @query('#end_process_btn', true)
    accessor _btn_end_process!: NativeButton;

    // Click handler for the "End Process" button that calls the native controller's endTask method with the PID and name of 
    // the currently selected process.
    private _onEndTask(_event: MouseEvent) {
        if(!this._selection) return;
        NProcessController.endTask(this._selection.pid, this._selection.name);
    }

    // Event handler for when the selected process in the table changes. 
    // Updates the _selection field and enables/disables buttons as necessary.
    private readonly _onSelectionChanged = (event: PTSelectionChangedEvent) => {
        this._selection = event.detail.new_selection;
        if(this._selection) {
            this._btn_end_process.classList.remove('disabled');
        }
        else {
            this._btn_end_process.classList.add('disabled');
        }
    }

    private _selection?: ProcessInfo;

    static styles = css`
        :host {
            display: flex;
            flex-direction: row;
            align-items: center;
        }

        wa-checkbox::part(control) {
            border: var(--check-unchecked-border);
            background-color: var(--check-unchecked-bg);
            align-self: center;
        }

        wa-checkbox::part(control):hover {
            border: var(--check-unchecked-hover-border);
            background-color: var(--check-unchecked-hover-bg);
        }

        wa-checkbox:state(checked)::part(control) {
            border: var(--check-checked-border);
            background-color: var(--check-checked-bg);
        }

        wa-checkbox::part(icon) {
            color: var(--check-checked-icon-color);
        }

        wa-checkbox:focus-visible {
            outline: var(--check-focus-visible);
            outline-offset: var(--check-focus-offset);
        }

        .left {
            flex: 1;
            display: flex;
            flex-direction: row;
            align-items: center;
            overflow: hidden;
            gap: var(--control-spacing);
        }

        .left > div {
            display: block;
            overflow: hidden;
            text-overflow: ellipsis;
            white-space: nowrap;
        }

        .right {
            margin-left: auto;
            display: flex;
            flex-direction: row;
            align-items: center;
        }
    `
}

declare global {
    interface HTMLElementTagNameMap {
        "pt-control": PTControl;
    }
}