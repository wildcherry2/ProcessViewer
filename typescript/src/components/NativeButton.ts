import { css, html, LitElement } from "lit";
import { customElement } from "lit/decorators.js";

@customElement('native-button')
export class NativeButton extends LitElement {
    protected override render() { // figure out :disabled state?
        return html`
            <button id="button"><slot></slot></button>
        `;
    }

    static styles = css`
        :host {
            display: contents;
            --default-button-height: 40px;
            --default-button-min-width: 80px;
            --p-button-height: var(--button-height, 40px);
            --p-button-min-width: var(--button-min-width, 80px);
            --p-button-padding: calc(var(--p-button-height) / 2)
        }

        :host > button {
            font-family: "Segoe UI Variable", "Segoe UI", sans-serif;
            font-size: 14px;
            font-weight: 500;
            
            height: var(--p-button-height);
            padding: 0 var(--p-button-padding);
            min-width: var(--p-button-min-width);
            border-radius: var(--button-radius);
            border: none;
            cursor: pointer;
            transition: var(--button-transition);
            user-select: none;
            outline: none;
            display: inline-flex;
            align-items: center;
            justify-content: center;
            gap: 8px;
        }

            /* Primary (accent) button */
        :host(.primary) > button {
            background: var(--accent-color);
            color: var(--button-text);
            /* box-shadow: var(--button-shadow); */
        }

        :host(.primary:hover) > button {
            background: var(--accent-hover);
        }

        :host(.primary:active) > button {
            background: var(--accent-pressed);
            /* box-shadow: inset 0 2px 4px rgba(0, 0, 0, 0.2); */
        }

        :host(.primary:focus-visible) > button {
            outline: 2px solid var(--accent-color);
            outline-offset: 2px;
        }

            /* Secondary (neutral) button */
        :host(.secondary) > button {
            background: var(--neutral-button-bg);
            color: var(--neutral-button-text);
            /* box-shadow: var(--button-shadow); */
        }

        :host(.secondary:hover) > button {
            background: var(--neutral-button-hover);
        }

        :host(.secondary:active) > button {
            background: var(--neutral-button-pressed);
            /* box-shadow: inset 0 2px 4px rgba(0, 0, 0, 0.2); */
        }

        :host(.secondary:focus-visible) > button {
            outline: 2px solid var(--accent-color);
            outline-offset: 2px;
        }

        :host(.disabled) > button {
            opacity: 0.5;
            cursor: not-allowed;
            /* box-shadow: none; */
        }
    `;
}

declare global {
    interface HTMLElementTagNameMap {
        "native-button": NativeButton;
    }
}