const path = require('path');
const HtmlWebpackPlugin = require('html-webpack-plugin');
const MiniCssExtractPlugin = require('mini-css-extract-plugin');
const CssMinimizerPlugin = require('css-minimizer-webpack-plugin');
const webpack = require('webpack');

module.exports = (env: any) => {
    const env_mode = env.production ? 'production' : 'development';
    const out_path = path.resolve(__dirname, env_mode === 'production' ? 'dist' : 'develop');
    console.log(`Building in ${env_mode} mode...`);
    console.log(`./tsconfig.${env_mode}.json`)
    return {
            entry: './src/index.ts', // Entry point of your TypeScript application
            output: {
                filename: 'bundle.js', // Name of the bundled output file
                path: out_path, // Output directory
                clean: true, // Clean the output directory before each build
            },
            plugins: [
                new HtmlWebpackPlugin({
                    template: './src/index.html', // Your custom template
                }),
                new webpack.ContextReplacementPlugin(
                    /@awesome\.me\/webawesome/,
                    path.resolve(__dirname, 'node_modules/@awesome.me/webawesome/dist')
                ),
                new webpack.DefinePlugin({
                    'BUILD_MODE': JSON.stringify(env_mode),
                    'ENABLE_LOGGING': JSON.stringify(!!env.enablelogging)
                }),
                ...(env_mode === 'production' ? [new MiniCssExtractPlugin({ filename: 'bundle.min.css' })] : []),
            ],
            resolve: {
                extensions: ['.ts', '.js'], // Resolve these file extensions when importing
            },
            module: {
                rules: [
                {
                    test: /\.tsx?$/, // Apply this rule to .ts and .tsx files
                    use: [
                        {
                            loader: 'ts-loader',
                            options: {
                                configFile: path.resolve(__dirname, `./tsconfig.${env_mode}.json`) 
                            }
                        }
                    ],
                    exclude: /node_modules/, // Exclude dependencies
                },
                {
                    test: /\.css$/, // Apply this rule to .css files
                    use: [env_mode === 'production' ? MiniCssExtractPlugin.loader : 'style-loader', 'css-loader'], // Use css-loader then style-loader
                },
                {
                    test: /\.(png|jpe?g|gif|svg)$/i, // Match common image file types
                    type: 'asset/resource',         // Tells Webpack to emit the file and return the URL
                },
                {
                    test: /\.(js|jsx|ts|tsx)$/,
                    use: [{
                        loader: 'minify-html-literals-loader',
                        options: {
                            minifyCSS: true
                        }
                    }]
                }
                ],
            },
            mode: env_mode, // Set Webpack mode (development or production),
            devtool: env_mode === 'development' ? 'source-map' : undefined,
            devServer: {
                static: out_path, // Serve static files from the 'dist' directory
                port: 3000, // Serve the app on http://localhost:3000
                open: true, // Automatically open the browser when the server starts
            },
            optimization: {
                minimize: env_mode === 'production',
                minimizer: [
                    '...', // keep the default JS minimizer
                    new CssMinimizerPlugin()
                ]
            }
        };
}