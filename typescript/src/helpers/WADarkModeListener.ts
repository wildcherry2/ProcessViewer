import { WeakArray } from "./WeakArray";

export {}

class WADarkModeUpdater {
    constructor() {
        const dark_mode = matchMedia("(prefers-color-scheme: dark)");
        this._is_dark = dark_mode.matches;
        this._apply();

        dark_mode.addEventListener("change", (event) => {
            this._is_dark = event.matches;
            this._apply();
        });
    }

    addElement<T extends (HTMLElement | null)>(element?: T) {
        if(!element) return;
        element.classList.toggle('wa-dark', this._is_dark);
        this._elems.push(element);
    }

    private _apply() {
        for(const elem of this._elems) {
            elem.classList.toggle('wa-dark', this._is_dark);
        }
    }

    private _is_dark: boolean;
    private _elems: WeakArray<HTMLElement> = new WeakArray();
}

(window['WebAwesomeDarkModeUpdater'] as any) ??= new WADarkModeUpdater();

declare global {
    interface Window {
        readonly WebAwesomeDarkModeUpdater: WADarkModeUpdater
    }
}