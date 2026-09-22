import js from '@eslint/js';
import ts from 'typescript-eslint';
export default ts.config(
 { ignores: ['**/dist/**', '**/node_modules/**', 'artifacts/**', 'docs/research/**'] },
 js.configs.recommended,
 ...ts.configs.recommended,
 { languageOptions: { globals: { console:'readonly', process:'readonly', Buffer:'readonly', URL:'readonly', setTimeout:'readonly', clearTimeout:'readonly' } }, rules: { '@typescript-eslint/no-unused-vars':['error',{argsIgnorePattern:'^_',varsIgnorePattern:'^_'}], '@typescript-eslint/no-explicit-any':'error' } }
);
