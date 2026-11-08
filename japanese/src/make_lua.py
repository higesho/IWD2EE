# IWD2JP.lua.in に禁則の機械語（kinsoku.hex）を埋め込んで IWD2JP.lua を作る
import os
here = os.path.dirname(os.path.abspath(__file__))
hexs = open(os.path.join(here, 'kinsoku.hex')).read().split()
body = '\n'.join('\t' + ' '.join(hexs[i:i + 32]) for i in range(0, len(hexs), 32))
src = open(os.path.join(here, 'IWD2JP.lua.in'), encoding='utf-8').read()
out = src.replace('@@HEX@@', body)
open(os.path.join(here, 'IWD2JP.lua'), 'w', encoding='utf-8', newline='\n').write(out)
print('ok', len(hexs))
