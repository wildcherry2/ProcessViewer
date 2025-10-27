import { css, html, LitElement, PropertyValues } from "lit";
import { customElement, property, query } from "lit/decorators.js";

import './NativeButton'
import '@awesome.me/webawesome/dist/components/checkbox/checkbox.js'

import type WaCheckbox from "@awesome.me/webawesome/dist/components/checkbox/checkbox.js";
import type { ProcessInfo, PTSelectionChangedEvent } from "../types";
import type { NativeButton } from "./NativeButton";

@customElement('pt-control')
export class PTControl extends LitElement {
    connectedCallback(): void {
        super.connectedCallback();
        document.addEventListener('pt-selection-changed', this._onSelectionChanged);
    }

    disconnectedCallback(): void {
        super.disconnectedCallback();
        document.removeEventListener('pt-selection-changed', this._onSelectionChanged);
    }

    protected override render() { //might need to normalize button lengths, and hide end task if nothing is selected, and figure out what to do with dll text
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

    private _onEndTask(_event: MouseEvent) {
        if(!this._selection) return;
        NProcessController.endTask(this._selection.pid, this._selection.name);
    }

    private readonly _onSelectionChanged = (event: PTSelectionChangedEvent) => {
        this._selection = event.detail.new_selection;
        if(this._selection) {
            this._btn_end_process.classList.remove('disabled');
        }
        else {
            this._btn_end_process.classList.add('disabled');
        }
    }

    private _selection?: ProcessInfo; // make @state and link to disabled classes?

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